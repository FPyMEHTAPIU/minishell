/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execution.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msavelie <msavelie@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/28 12:04:25 by msavelie          #+#    #+#             */
/*   Updated: 2025/02/13 15:23:12 by msavelie         ###   ########.fr       */
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
		if (isatty(STDIN_FILENO))
			printf("exit\n");
		clean_mshell(obj);
		free(obj->envp);
		exit(obj->exit_code);
	}
	else if (args_len >= 2)
	{
		if (isatty(STDIN_FILENO))
			printf("exit\n");
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
		obj->exit_code = ft_atoi(args[1]);
		// if (obj->exit_code == 0)
		// {
		// 	// if (obj->exit_code < 0)
		// 	// 	obj->exit_code = 156;
		// }
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
		return(echo(args, obj));
	else if (ft_strcmp(args[0], "env") == 0)
	 	return (env(obj));
	else if (ft_strcmp(args[0], "cd") == 0)
		return (cd(args, obj));
	else if (ft_strcmp(args[0], "export") == 0)
		return (export(args, obj));
	else if (ft_strcmp(args[0], "unset") == 0)
		return (unset(args, obj));
	else if (ft_strcmp(args[0], "pwd") == 0)
		return(pwd(obj));
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
		if (cmd->redirs[i]->type == TOKEN_HEREDOC
			|| cmd->redirs[i]->type == TOKEN_REDIRECT_IN)
			redirection_input(obj, cmd->redirs[i]);
		else if (cmd->redirs[i]->type == TOKEN_REDIRECT_OUT ||
				cmd->redirs[i]->type == TOKEN_REDIRECT_APPEND)
			redirection_output(obj, cmd->redirs[i]);
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
		return ;
	}
	else if (obj->pids[obj->cur_pid] == 0)
	{
		reset_signals();
		restore_terminal_settings();
		apply_redirections(obj, cmd);

		/* Abort command execution if heredoc was interrupted */
		if (obj->stdin_fd != -1)
			close(obj->stdin_fd);
		if (obj->heredoc_interrupted)
			exit_child(obj, "", 130, 0);

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

static void	run_heredoc(t_mshell *obj, t_ast_node *node)
{
	int	i;
	int	last_fd;
	int	is_last_heredoc;

	if (!node || !node->redirs || !*node->redirs)
		return ;
	i = 0;
	last_fd = -1;
	obj->stdin_fd = dup(STDIN_FILENO);
	is_last_heredoc = 0;
	while (node->redirs[i])
	{
		if (last_fd != -1 &&
			(node->redirs[i]->type == TOKEN_REDIRECT_IN || node->redirs[i]->type == TOKEN_HEREDOC))
		{
			close(last_fd);
			last_fd = -1;
		}
		if (node->redirs[i]->type == TOKEN_REDIRECT_IN)
			is_last_heredoc = 0;
		else if (node->redirs[i]->type == TOKEN_HEREDOC)
			is_last_heredoc = 1;
		last_fd = handle_here_doc(obj, node->redirs[i], last_fd);
		i++;
	}
	if (last_fd != -1 && i > 0 && is_last_heredoc == 1)
	{
		dup2(last_fd, STDIN_FILENO);
		close(last_fd);
	}
	else if (last_fd != -1)
		close(last_fd);
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
		if (obj->heredoc_interrupted || g_signal_received)
			break;
		if (temp->left)
		{
			run_heredoc(obj, temp->left);
			execute_cmd(obj, temp->left);
		}
		else
		{
			run_heredoc(obj, temp);
			execute_cmd(obj, temp);
		}
		if (obj->stdin_fd != -1)
		{
			dup2(obj->stdin_fd, STDIN_FILENO);
			close(obj->stdin_fd);
			obj->stdin_fd = -1;
		}
		temp = temp->right;
		obj->cur_pid++;
	}
}
