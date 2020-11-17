#include "fileutils.h"

#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef _POSIX_SPAWN
#include <poll.h>
#include <spawn.h>
#else
#include <signal.h>
#endif

#include "def.h"
#include "dialog.h"
#include "sdlutils.h"

namespace
{

int WaitPid(pid_t id)
{
    int status, ret;
    while ((ret = waitpid(id, &status, WNOHANG | WUNTRACED)) == 0)
        usleep(50 * 1000);
    if (ret < 0) perror("waitpid");
    return (WIFEXITED(status) != 0) ? WEXITSTATUS(status) : 0;
}

int SpawnAndWait(const char *argv[], std::string *capture_stdout = nullptr,
    std::string *capture_stderr = nullptr)
{
    pid_t child_pid;
#ifdef _POSIX_SPAWN
    std::vector<std::array<int, 2>> pipes;
    std::vector<std::string *> captures;
    posix_spawn_file_actions_t child_fd_actions;
    int ret;
    if (ret = ::posix_spawn_file_actions_init(&child_fd_actions)) return ret;
    const auto add_capture_pipe = [&](std::string *capture, int child_fd) {
        if (capture == nullptr) return 0;
        captures.push_back(capture);
        pipes.emplace_back();
        if (ret = ::pipe(pipes.back().data())) return ret;
        if (ret = ::posix_spawn_file_actions_addclose(
                &child_fd_actions, pipes.back()[0]))
            return ret;
        if (ret = ::posix_spawn_file_actions_adddup2(
                &child_fd_actions, pipes.back()[1], child_fd))
            return ret;
        return 0;
    };
    if (ret = add_capture_pipe(capture_stdout, 1)) return ret;
    if (ret = add_capture_pipe(capture_stderr, 2)) return ret;

    // This const cast is OK, see https://stackoverflow.com/a/190208.
    if (ret = ::posix_spawnp(&child_pid, argv[0], &child_fd_actions, nullptr,
            (char **)argv, nullptr))
        return ret;

    // Read from pipes
    if (!pipes.empty())
    {
        for (auto &p : pipes) ::close(p[1]);
        constexpr std::size_t kBufferSize = 1024;
        std::unique_ptr<char[]> buffer { new char[kBufferSize] };
        std::vector<::pollfd> poll_fds;
        for (auto &p : pipes) poll_fds.push_back(::pollfd { p[0], POLL_IN });
        while ((ret = ::poll(&poll_fds[0], poll_fds.size(), -1)) >= 0)
        {
            bool got_data = false;
            for (std::size_t i = 0; i < poll_fds.size(); ++i)
            {
                if ((poll_fds[i].revents & POLL_IN) == 0) continue;
                const int bytes_read
                    = ::read(pipes[i][0], buffer.get(), kBufferSize);
                got_data = true;
                if (bytes_read > 0)
                    captures[i]->append(buffer.get(), bytes_read);
            }
            if (!got_data) break; // nothing left to read
        }
        for (auto &p : pipes) ::close(p[0]);
    }

    ret = WaitPid(child_pid);
    ::posix_spawn_file_actions_destroy(&child_fd_actions);
    return ret;
#else
    struct sigaction sa, save_quit, save_int;
    sigset_t save_mask;
    int wait_val;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    /* __sigemptyset(&sa.sa_mask); - done by memset() */
    /* sa.sa_flags = 0; - done by memset() */

    sigaction(SIGQUIT, &sa, &save_quit);
    sigaction(SIGINT, &sa, &save_int);
    sigaddset(&sa.sa_mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sa.sa_mask, &save_mask);
    if ((child_pid = vfork()) < 0)
    {
        wait_val = -1;
        goto out;
    }
    if (child_pid == 0)
    {
        sigaction(SIGQUIT, &save_quit, NULL);
        sigaction(SIGINT, &save_int, NULL);
        sigprocmask(SIG_SETMASK, &save_mask, NULL);

        // This const cast is OK, see https://stackoverflow.com/a/190208.
        if (execvp(argv[0], (char **)argv) == -1) perror(argv[0]);
        exit(127);
    }
    wait_val = WaitPid(child_pid);
out:
    sigaction(SIGQUIT, &save_quit, NULL);
    sigaction(SIGINT, &save_int, NULL);
    sigprocmask(SIG_SETMASK, &save_mask, NULL);
    return wait_val;
#endif
}

const char *AsConstCStr(const char *s) { return s; }
const char *AsConstCStr(const std::string &s) { return s.c_str(); }

class ActionResult
{
    public:
    ActionResult(int errnum, std::string error_message)
        : errnum_(errnum)
        , message_(std::move(error_message))
    {
        if (errnum != 0 && message_.empty())
            message_.append(std::strerror(errnum));
    }

    bool ok() const { return errnum_ == 0; }

    const std::string &message() const { return message_; }

    private:
    int errnum_;
    std::string message_;
};

template <typename... Args> ActionResult Run(Args... args)
{
    const char *execve_args[] = { AsConstCStr(args)..., nullptr };
    std::string err;
    const int errnum = SpawnAndWait(
        execve_args, /*capture_stdout=*/nullptr, /*capture_stderr=*/&err);
    if (errnum == 0) { err.clear(); }
    else if (!err.empty() && err.back() == '\n')
    {
        err.resize(err.size() - 1);
    }
    return ActionResult { errnum, std::move(err) };
}

void JoinPath(const std::string &a, const std::string &b, std::string &out)
{
    out = a;
    if (a.back() != '/') out += '/';
    out.append(b);
}

enum class OverwriteDialogResult
{
    YES,
    YES_TO_ALL,
    NO,
    CANCEL
};
OverwriteDialogResult OverwriteDialog(
    const std::string &dest_filename, bool is_last)
{
    CDialog dlg("File already exists:");
    dlg.addLabel("Overwrite " + dest_filename + "?");
    std::vector<OverwriteDialogResult> options {
        OverwriteDialogResult::CANCEL
    };
    const auto add_option = [&](std::string text, OverwriteDialogResult value) {
        dlg.addOption(text);
        options.push_back(value);
    };
    add_option("Yes", OverwriteDialogResult::YES);
    if (!is_last) add_option("Yes to all", OverwriteDialogResult::YES_TO_ALL);
    add_option("No", OverwriteDialogResult::NO);
    if (!is_last) add_option("Cancel", OverwriteDialogResult::CANCEL);
    dlg.init();
    auto res = options[dlg.execute()];
    SDL_utils::renderAll();
    return res;
}

using ActionFn = std::function<ActionResult(
    const std::string & /*src*/, const std::string & /*dest*/)>;

enum class ErrorDialogResult
{
    ABORT,
    CONTINUE
};
ErrorDialogResult ErrorDialog(
    const std::string &action, const std::string &error, bool is_last)
{
    CDialog dlg("Error:");
    dlg.addLabel(action);
    dlg.addLabel(error);
    std::vector<ErrorDialogResult> options { ErrorDialogResult::ABORT };
    const auto add_option = [&](std::string text, ErrorDialogResult value) {
        dlg.addOption(text);
        options.push_back(value);
    };
    if (!is_last)
    {
        add_option("Continue", ErrorDialogResult::CONTINUE);
        add_option("Abort", ErrorDialogResult::ABORT);
    }
    else
    {
        add_option("OK", ErrorDialogResult::ABORT);
    }
    dlg.init();
    return options[dlg.execute()];
}

using ProgressFn = std::function<void(
    const std::string & /*desc*/, std::size_t /*i*/, std::size_t /*n*/)>;

// No-op for now.
// TODO: Implement UI progress updates
void DefaultProgressFn(
    const std::string &description, std::size_t index, std::size_t count)
{
}

void ActionToDir(const std::vector<std::string> &inputs,
    const std::string &dest_dir, const char *description, ActionFn action_fn,
    ProgressFn progress_fn = &DefaultProgressFn)
{
    bool confirm_overwrite = true;
    std::string dest_filename, action_desc;
    std::size_t i = 0;
    for (const std::string &input : inputs)
    {
        action_desc = description;
        action_desc += ' ';
        action_desc.append(File_utils::getFileName(input));
        const bool is_last = (i == input.size() - 1);
        progress_fn(action_desc, i++, inputs.size());
        JoinPath(dest_dir, File_utils::getFileName(input), dest_filename);
        if (confirm_overwrite && File_utils::fileExists(dest_filename))
        {
            switch (OverwriteDialog(dest_filename, is_last))
            {
                case OverwriteDialogResult::YES: break;
                case OverwriteDialogResult::YES_TO_ALL:
                    confirm_overwrite = false;
                    break;
                case OverwriteDialogResult::NO: continue;
                case OverwriteDialogResult::CANCEL: return;
            }
        }
        const auto action_result = action_fn(input, dest_filename);
        if (!action_result.ok())
        {
            switch (ErrorDialog(action_desc, action_result.message(), is_last))
            {
                case ErrorDialogResult::CONTINUE: continue;
                case ErrorDialogResult::ABORT: return;
            }
        }
    }
}

} // namespace

void File_utils::copyFile(
    const std::vector<std::string> &srcs, const std::string &dest_dir)
{
    ActionToDir(srcs, dest_dir, "Copying",
        [&](const std::string &src, const std::string & /*dest*/) {
            return Run("cp", "-f", "-r", src, dest_dir);
        });
    Run("sync", dest_dir);
}

void File_utils::moveFile(
    const std::vector<std::string> &srcs, const std::string &dest_dir)
{
    ActionToDir(srcs, dest_dir, "Moving",
        [&](const std::string &src, const std::string &dest) {
            return Run("mv", "-f", src, dest_dir);
        });
    Run("sync", dest_dir);
}

void File_utils::symlinkFile(
    const std::vector<std::string> &srcs, const std::string &dest_dir)
{
    ActionToDir(srcs, dest_dir, "Creating symlink",
        [&](const std::string &src, const std::string &dest) {
            return Run("ln", "-sf", src, dest_dir);
        });
    Run("sync", dest_dir);
}

void File_utils::renameFile(
    const std::string &p_file1, const std::string &p_file2)
{
    if (!fileExists(p_file2)
        || OverwriteDialog(p_file2, /*is_last=*/true)
            == OverwriteDialogResult::YES)
    {
        auto result = Run("mv", "-f", p_file1, p_file2);
        if (result.ok()) result = Run("sync", p_file2);
        if (!result.ok())
        {
            ErrorDialog("Renaming " + getFileName(p_file1) + " to "
                    + getFileName(p_file2),
                result.message(), /*is_last=*/true);
        }
    }
}

void File_utils::removeFile(const std::vector<std::string> &p_files)
{
    for (const std::string &path : p_files)
    {
        auto result = Run("rm", "-rf", path);
        if (!result.ok())
        {
            switch (ErrorDialog("Removing " + path, result.message(),
                /*is_last=*/&path == &p_files.back()))
            {
                case ErrorDialogResult::CONTINUE: continue;
                case ErrorDialogResult::ABORT: return;
            }
        }
    }
}

void File_utils::makeDirectory(const std::string &p_file)
{
    auto result = Run("mkdir", "-p", p_file);
    if (result.ok()) result = Run("sync", p_file);
    if (!result.ok())
        ErrorDialog("Creating " + p_file, result.message(), /*is_last=*/true);
}

const bool File_utils::fileExists(const std::string &p_path)
{
    struct stat l_stat;
    return stat(p_path.c_str(), &l_stat) == 0;
}

static void AsciiToLower(std::string *s)
{
    for (char &c : *s)
        if (c >= 'A' && c <= 'Z') c -= ('Z' - 'z');
}

std::string File_utils::getLowercaseFileExtension(const std::string &name)
{
    const auto dot_pos = name.rfind('.');
    if (dot_pos == std::string::npos) return "";
    std::string ext = name.substr(dot_pos + 1);
    AsciiToLower(&ext);
    return ext;
}

const std::string File_utils::getFileName(const std::string &p_path)
{
    size_t l_pos = p_path.rfind('/');
    return p_path.substr(l_pos + 1);
}

const std::string File_utils::getPath(const std::string &p_path)
{
    size_t l_pos = p_path.rfind('/');
    return p_path.substr(0, l_pos);
}

void File_utils::executeFile(const std::string &p_file)
{
    // Command
    INHIBIT(std::cout << "File_utils::executeFile: " << p_file << " in "
                      << getPath(p_file) << std::endl;)
    // CD to the file's location
    chdir(getPath(p_file).c_str());
    // Quit
    SDL_utils::hastalavista();
    // Execute file
    if (getLowercaseFileExtension(p_file) == "opk")
    {
        execlp("opkrun", "opkrun", p_file.c_str(), nullptr);
    }
    else
    {
        execl(p_file.c_str(), p_file.c_str(), nullptr);
    }
    // If we're here, there was an error with the execution
    perror("Child process error");
    std::cerr << "Error executing file " << p_file << std::endl;
    // Relaunch DinguxCommander
    const std::string self_name = getSelfExecutionName();
    INHIBIT(std::cout << "File_utils::executeFile: " << self_name << " in "
                      << getSelfExecutionPath() << std::endl;)
    chdir(getSelfExecutionPath().c_str());
    execl(self_name.c_str(), self_name.c_str(), NULL);
}

const std::string File_utils::getSelfExecutionPath(void)
{
    // Get execution path
    std::string l_exePath("");
    char l_buff[255];
    int l_i = readlink("/proc/self/exe", l_buff, 255);
    l_exePath = l_buff;
    l_exePath = l_exePath.substr(0, l_i);
    l_i = l_exePath.rfind("/");
    l_exePath = l_exePath.substr(0, l_i);
    return l_exePath;
}

const std::string File_utils::getSelfExecutionName(void)
{
    // Get execution path
    std::string l_exePath("");
    char l_buff[255];
    int l_i = readlink("/proc/self/exe", l_buff, 255);
    l_exePath = l_buff;
    l_exePath = l_exePath.substr(0, l_i);
    l_i = l_exePath.rfind("/");
    l_exePath = l_exePath.substr(l_i + 1);
    return l_exePath;
}

void File_utils::stringReplace(std::string &p_string,
    const std::string &p_search, const std::string &p_replace)
{
    // Replace all occurrences of p_search by p_replace in p_string
    size_t l_pos = p_string.find(p_search, 0);
    while (l_pos != std::string::npos)
    {
        p_string.replace(l_pos, p_search.length(), p_replace);
        l_pos = p_string.find(p_search, l_pos + p_replace.length());
    }
}

const unsigned long int File_utils::getFileSize(const std::string &p_file)
{
    unsigned long int l_ret(0);
    struct stat l_stat;
    if (stat(p_file.c_str(), &l_stat) == -1)
        std::cerr << "File_utils::getFileSize: Error stat " << p_file
                  << std::endl;
    else
        l_ret = l_stat.st_size;
    return l_ret;
}

void File_utils::diskInfo(void)
{
    std::string l_line("");
    SDL_utils::pleaseWait();
    // Execute command df -h
    {
        char l_buffer[256];
        FILE *l_pipe = popen("df -h " FILE_SYSTEM, "r");
        if (l_pipe == NULL)
        {
            std::cerr << "File_utils::diskInfo: Error popen" << std::endl;
            return;
        }
        while (
            l_line.empty() && fgets(l_buffer, sizeof(l_buffer), l_pipe) != NULL)
            if (strstr(l_buffer, FILE_SYSTEM) != NULL) l_line = l_buffer;
        pclose(l_pipe);
    }
    if (!l_line.empty())
    {
        // Separate line by spaces
        std::istringstream l_iss(l_line);
        std::vector<std::string> l_tokens;
        std::copy(std::istream_iterator<std::string>(l_iss),
            std::istream_iterator<std::string>(),
            std::back_inserter<std::vector<std::string>>(l_tokens));
        // Display dialog
        CDialog l_dialog("Disk information:", 0, 0);
        l_dialog.addLabel("Size: " + l_tokens[1]);
        l_dialog.addLabel("Used: " + l_tokens[2] + " (" + l_tokens[4] + ")");
        l_dialog.addLabel("Available: " + l_tokens[3]);
        l_dialog.addOption("OK");
        l_dialog.init();
        l_dialog.execute();
    }
    else
        std::cerr << "File_utils::diskInfo: Unable to find " << FILE_SYSTEM
                  << std::endl;
}

void File_utils::diskUsed(const std::vector<std::string> &p_files)
{
    std::string l_line("");
    // Waiting message
    SDL_utils::pleaseWait();
    // Build and execute command
    {
        std::string l_command("du -csh");
        for (std::vector<std::string>::const_iterator l_it = p_files.begin();
             l_it != p_files.end(); ++l_it)
            l_command = l_command + " \"" + *l_it + "\"";
        char l_buffer[256];
        FILE *l_pipe = popen(l_command.c_str(), "r");
        if (l_pipe == NULL)
        {
            std::cerr << "File_utils::diskUsed: Error popen" << std::endl;
            return;
        }
        while (fgets(l_buffer, sizeof(l_buffer), l_pipe) != NULL)
            ;
        l_line = l_buffer;
        pclose(l_pipe);
    }
    // Separate line by spaces
    {
        std::istringstream l_iss(l_line);
        std::vector<std::string> l_tokens;
        std::copy(std::istream_iterator<std::string>(l_iss),
            std::istream_iterator<std::string>(),
            std::back_inserter<std::vector<std::string>>(l_tokens));
        l_line = l_tokens[0];
    }
    // Dialog
    std::ostringstream l_stream;
    CDialog l_dialog("Disk used:", 0, 0);
    l_stream << p_files.size() << " items selected";
    l_dialog.addLabel(l_stream.str());
    l_dialog.addLabel("Disk used: " + l_line);
    l_dialog.addOption("OK");
    l_dialog.init();
    l_dialog.execute();
}

void File_utils::formatSize(std::string &p_size)
{
    // Format 123456789 to 123,456,789
    int l_i = p_size.size() - 3;
    while (l_i > 0)
    {
        p_size.insert(l_i, ",");
        l_i -= 3;
    }
}
