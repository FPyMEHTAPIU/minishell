/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execution.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rkhakimu <rkhakimu@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/28 12:04:25 by msavelie          #+#    #+#             */
/*   Updated: 2025/02/07 17:50:54 by rkhakimu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/minishell.h"

static void	check_and_handle_exit(char **args, t_mshell *obj)
{
	int	i;
	int	args_len;

	if (!args || !*args)
		return ;
	args_len = 0;
	while (args[args_len])
		args_len++;
	if (args_len == 1)
	{
		//printf("exit\n");
		clean_mshell(obj);
		free(obj->envp);
		exit(obj->exit_code);
	}
	else if (args_len >= 2)
	{
		//printf("exit\n");
		i = 0;
		while (args[1][i])
		{
			if (ft_isdigit(args[1][i]) == 0 && args[1][i] != '-' && args[1][i] != '+')
			{
				obj->exit_code = 2;
				ft_fprintf(2, "minishell: exit: %s: numeric argument required\n", args[1]);
				clean_mshell(obj);
				free(obj->envp);
				exit(obj->exit_code);
			}
			i++;
		}
		if (args_len > 2)
		{
			obj->exit_code = 1;
			ft_fprintf(2, "minishell: exit: too many arguments\n");
			clean_mshell(obj);
			free(obj->envp);
			exit(obj->exit_code);
		}
		if (obj->exit_code == 0)
		{
			obj->exit_code = ft_atoi(args[1]);
			// if (obj->exit_code < 0)
			// 	obj->exit_code = 156;
		}
		clean_mshell(obj);
		//free(obj->envp);
		exit(obj->exit_code);
	}
}

static int	is_builtin_cmd(char *cmd)
{
	if (ft_strcmp(cmd, "echo") == 0 
		|| ft_strcmp(cmd, "env") == 0
		|| ft_strcmp(cmd, "cd") == 0
		|| ft_strcmp(cmd, "export") == 0
		|| ft_strcmp(cmd, "unset") == 0
		|| ft_strcmp(cmd, "pwd") == 0
		|| ft_strcmp(cmd, "exit") == 0)
		return (1);
	return (0);
}

void	exit_child(t_mshell *obj, char *arg, int exit_code, int is_builtin)
{
	obj->exit_code = exit_code;
	close_fds(obj);
	clean_mshell(obj);
	if (!*arg)
		ft_putstr_fd(": ", 2);
	if (obj->exit_code != 0 && is_builtin == 0)
		perror(arg);
	if (errno == EACCES && obj->exit_code != 1)
		obj->exit_code = 126;
	exit(obj->exit_code);
}

static int	run_builtins(char **args, t_mshell *obj)
{
	if (!args || !*args)
		return (0);
	if (ft_strcmp(args[0], "echo") == 0)
		return(echo(args));
	else if (ft_strcmp(args[0], "env") == 0)
	 	return (env(obj));
	else if (ft_strcmp(args[0], "cd") == 0)
		return (cd(args, obj));
	else if (ft_strcmp(args[0], "export") == 0)
		return (export(args, obj));
	else if (ft_strcmp(args[0], "unset") == 0)
		return (unset(args, obj));
	else if (ft_strcmp(args[0], "pwd") == 0)
		return(pwd());
	else if (ft_strcmp(args[0], "exit") == 0)
	{
		check_and_handle_exit(args, obj);
		return (1);
	}
	return (0);
}

void	alloc_pipes(t_mshell *obj)
{
	int		i;

	if (!obj)
		return ;
	obj->pipfd = ft_calloc(obj->pipes_count, sizeof(int *));
	if (!obj->pipfd)
	{
		clean_mshell(obj);
		error_ret(5, NULL);
	}
	i = 0;
	while (i < obj->pipes_count)
	{
		obj->pipfd[i] = ft_calloc(2, sizeof(int));
		if (!obj->pipfd[i])
		{
			clean_mshell(obj);
			error_ret(5, NULL);
		}
		obj->pipfd[i][0] = -1;
		obj->pipfd[i][1] = -1;
		if (pipe(obj->pipfd[i]) == -1)
		{
			clean_mshell(obj);
			error_ret(3, NULL);
		}
		obj->allocated_pipes++;
		i++;
	}
}

static void apply_redirections(t_mshell *obj, t_ast_node *cmd)
{
    int i;

    if (!cmd || !cmd->redirs)
        return;
    i = 0;
    while (cmd->redirs[i])
    {
        if (cmd->redirs[i]->type == TOKEN_HEREDOC)
        {
            /* For heredoc, write temporary file and then redirect input */
            handle_here_doc(obj, cmd->redirs[i]);
            redirection_input(obj, cmd->redirs[i]);
        }
        else if (cmd->redirs[i]->type == TOKEN_REDIRECT_IN)
        {
            redirection_input(obj, cmd->redirs[i]);
        }
        else if (cmd->redirs[i]->type == TOKEN_REDIRECT_OUT ||
                 cmd->redirs[i]->type == TOKEN_REDIRECT_APPEND)
        {
            redirection_output(obj, cmd->redirs[i]);
        }
        i++;
    }
}

void execute_cmd(t_mshell *obj, t_ast_node *cmd)
{
    if (!cmd || !cmd->args || !cmd->args[0])
		return;
    if (obj->allocated_pipes == 0 && obj->redir_check == 0 && run_builtins(cmd->args, obj) == 1)
		return;
    obj->args_move = 0;
    obj->exec_cmds++;
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    obj->pids[obj->cur_pid] = fork();
    if (obj->pids[obj->cur_pid] == -1)
    {
        clean_mshell(obj);
        return;
    }
    else if (obj->pids[obj->cur_pid] == 0)
    {
        reset_signals();
        restore_terminal_settings();
        /* Instead of checking various redir node types, simply apply redirections */
        apply_redirections(obj, cmd);
        if (obj->allocated_pipes >= 1)
            pipe_redirection(obj, cmd);
        close_fds(obj);
        /* Execute builtins or external command */
        if (is_builtin_cmd(cmd->args[0]) == 1)
        {
            run_builtins(cmd->args, obj);
            exit_child(obj, cmd->args[0], obj->exit_code, 1);
        }
        else
        {
            obj->cur_path = check_paths_access(obj->paths, cmd, obj);
            execve(obj->cur_path, cmd->args + obj->args_move, obj->paths);
            exit_child(obj, cmd->args[0], 127, 0);
        }
    }
}

static void	check_redirections(t_mshell *obj)
{
	t_token	*temp;

	temp = obj->token;
	obj->redir_check = 0;
	while (temp)
	{
		if (temp->type == TOKEN_HEREDOC
			|| temp->type == TOKEN_REDIRECT_IN
			|| temp->type == TOKEN_REDIRECT_OUT
			|| temp->type == TOKEN_REDIRECT_APPEND)
		{
			obj->redir_check = 1;
			return ;
		}
		temp = temp->next;
	}
}

void choose_actions(t_mshell *obj)
{
    t_ast_node *temp;

    if (!obj)
        return;
    alloc_pipes(obj);
    check_redirections(obj);
    obj->pids = ft_calloc(obj->allocated_pipes + 1, sizeof(pid_t));
    if (!obj->pids)
    {
        clean_mshell(obj);
        error_ret(5, NULL);
    }
    temp = obj->ast;
    while (temp)
    {
        if (g_signal_received)
            break;
		if (temp->left)
			execute_cmd(obj, temp->left);
		else
			execute_cmd(obj, temp);
        temp = temp->right;
        obj->cur_pid++;
    }
}
