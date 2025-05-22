/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: atucci <atucci@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/19 16:52:08 by atucci            #+#    #+#             */
/*   Updated: 2025/05/22 14:53:56 by atucci           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// CgiExecutor.cpp
#include "Cgi.hpp"
#include <unistd.h>     // pipe, fork, dup2, execve, read, close
#include <sys/wait.h>   // waitpid, WEXITSTATUS
#include <sstream>      // std::ostringstream
#include <stdexcept>    // std::runtime_error
#include <cstring>      // strerror
#include <errno.h>

Cgi::Cgi() {}
Cgi::~Cgi() {}

// Public API
std::string Cgi::execute(const std::string& scriptPath)
{
    if (scriptPath.empty())
        throw std::runtime_error("Cgi: empty script path");
    return runForkExec(scriptPath.c_str());
}

// Core implementation
std::string Cgi::runForkExec(const char* script)
{
    int tubes[2];
    if (pipe(tubes) == -1) {
        throw std::runtime_error(std::string("pipe failed: ") + strerror(errno));
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(tubes[0]); close(tubes[1]);
        throw std::runtime_error(std::string("fork failed: ") + strerror(errno));
    }

    if (pid == 0) {
        // --- Child ---
        close(tubes[0]);                  // no reading in child
        if (dup2(tubes[1], STDOUT_FILENO) < 0)
            _exit(1);
        close(tubes[1]);

        // exec script (kernel interprets shebang)
        const char* args[] = { script, NULL };
        execve(script,
                const_cast<char* const*>(args),
                envp_);
        // if we get here, exec failed
        _exit(1);
    }

    // --- Parent ---
    close(tubes[1]);  // no writing in parent

    // Read child output
    std::string result;
    const int BUF = 256;
    char buf[BUF];
    ssize_t n;
    while ((n = read(tubes[0], buf, BUF-1)) > 0) {
        buf[n] = '\0';
        result += buf;
    }
    close(tubes[0]);

    // Reap child
    int status = 0;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        std::ostringstream oss;
        oss << "CGI script `" << script
            << "` exited with status " << WEXITSTATUS(status);
        throw std::runtime_error(oss.str());
    }

    return result;
}

