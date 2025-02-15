/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handling_signals.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rkhakimu <rkhakimu@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/13 16:18:33 by rkhakimu          #+#    #+#             */
/*   Updated: 2025/02/13 19:14:30 by rkhakimu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/minishell.h"

volatile sig_atomic_t g_signal_received = 0;
static struct termios original_term;
static struct termios shell_term;


static void set_sigaction(int signum, void (*handler)(int), int flags)
{
    struct sigaction sa;
    if (sigemptyset(&sa.sa_mask) == -1)
    {
        perror("sigemptyset error");
        exit(EXIT_FAILURE);
    }
    sa.sa_flags = flags;
    sa.sa_handler = handler;
    if (sigaction(signum, &sa, NULL) == -1)
    {
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }
}


static void interactive_signal_handler(int signum)
{
    struct termios t;
    g_signal_received = signum;
    if (signum == SIGINT)
    {
        if (write(STDOUT_FILENO, "\n", 1) == -1)
            restore_terminal_settings();
        if(tcgetattr(STDIN_FILENO, &t) == 0)
        {
            t.c_lflag |= (ECHO | ICANON);
            if(tcsetattr(STDIN_FILENO, TCSANOW, &t) == -1)
                perror("tcsetattr error in signal handler");
        }
        rl_on_new_line();
        rl_replace_line("", 0);
        rl_redisplay();
        ioctl(STDIN_FILENO, TCFLSH, 0);
    }
}

static void exec_signal_handler(int signum)
{
    g_signal_received = signum;
    if (signum == SIGINT)
    {
        if (write(STDERR_FILENO, "\n", 1) == -1)
            {}
    }
    else if (signum == SIGQUIT)
    {
        if (write(STDERR_FILENO, "Quit: (core dumped)\n", 21) == -1)
            {}
    }
}

void init_terminal_settings(void)
{
    if (!isatty(STDIN_FILENO))
        return;
        
    if (tcgetattr(STDIN_FILENO, &original_term) == -1)
    {
        perror("tcgetattr error");
        return;
    }
    shell_term = original_term;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &shell_term) == -1)
    {
        perror("tcsetattr error");
        return;
    }
}

void setup_interactive_signals(void)
{
    set_sigaction(SIGINT, interactive_signal_handler, 0);
    set_sigaction(SIGQUIT, SIG_IGN, 0);
}

void setup_exec_signals(void)
{
    set_sigaction(SIGINT, exec_signal_handler, 0);
    set_sigaction(SIGQUIT, exec_signal_handler, 0);
}

void setup_heredoc_signals(void)
{
    set_sigaction(SIGINT, interactive_signal_handler, 0);
    set_sigaction(SIGQUIT, SIG_IGN, 0);
}

void reset_signals(void)
{
    set_sigaction(SIGINT, SIG_DFL, 0);
    set_sigaction(SIGQUIT, SIG_DFL, 0);
}

void restore_terminal_settings(void)
{
    if (!isatty(STDIN_FILENO))
        return;
        
    if (tcsetattr(STDIN_FILENO, TCSANOW, &original_term) == -1)
    {
        perror("tcsetattr restore error");
    }
}

void transition_signal_handlers(t_signal_state new_state)
{

    if (new_state == SIGNAL_STATE_INTERACTIVE)
        setup_interactive_signals();
    else if (new_state == SIGNAL_STATE_EXEC)
        setup_exec_signals();
    else if (new_state == SIGNAL_STATE_HEREDOC)
        setup_heredoc_signals();
    else
        reset_signals();
}