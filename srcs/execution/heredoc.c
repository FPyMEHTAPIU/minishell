/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msavelie <msavelie@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/12 13:04:41 by msavelie          #+#    #+#             */
/*   Updated: 2025/02/13 15:01:53 by msavelie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/minishell.h"

static t_heredoc	init_heredoc(t_mshell *obj)
{
	t_heredoc	heredoc_obj;

	heredoc_obj.str = NULL;
	heredoc_obj.trimmed = NULL;
	heredoc_obj.expanded = NULL;
	heredoc_obj.obj = obj;
	heredoc_obj.pipe_fd[0] = -1;
	heredoc_obj.pipe_fd[1] = -1;
	if (pipe(heredoc_obj.pipe_fd) == -1)
	{
		clean_mshell(obj);
		error_ret(3, NULL);
	}
	return (heredoc_obj);
}

static void	cleanup_heredoc(t_heredoc *doc)
{
	if (!doc)
		return ;
	if (doc->str)
		free(doc->str);
	if (doc->trimmed)
		free(doc->trimmed);
	if (doc->expanded)
		free(doc->expanded);
	doc->str = NULL;
	doc->trimmed = NULL;
	doc->expanded = NULL;
}

static int	process_heredoc_line(t_heredoc *doc)
{
	write(STDOUT_FILENO, "> ", 2);
	doc->str = get_next_line(STDIN_FILENO);
	if (!doc->str || g_signal_received == SIGINT)
		return (0);
	doc->expanded = expand_env_vars(doc->str, doc->obj);
	doc->trimmed = ft_strtrim(doc->expanded, "\n");
	return (1);
}

static void	write_heredoc_line(t_heredoc *doc)
{
	if (doc->str[0] == '$')
		ft_fprintf(doc->pipe_fd[1], "%s", doc->expanded);
	else
		ft_fprintf(doc->pipe_fd[1], "%s", doc->str);
}

int	handle_here_doc(t_mshell *obj, t_ast_node *node, int last_fd)
{
	int			ret_fd;
	t_heredoc	heredoc;

	if (node->type != TOKEN_HEREDOC)
		return (last_fd);
	heredoc = init_heredoc(obj);
	g_signal_received = 0;
	while (process_heredoc_line(&heredoc))
	{
		if (!ft_strcmp(node->args[0], heredoc.trimmed) || 
			!ft_strcmp(node->args[0], heredoc.expanded))
			break;
		write_heredoc_line(&heredoc);
		cleanup_heredoc(&heredoc);
	}
	cleanup_heredoc(&heredoc);
	close(heredoc.pipe_fd[1]);
	ret_fd = heredoc.pipe_fd[0];
	if (g_signal_received == SIGINT)
	{
		obj->heredoc_interrupted = 1;
		return (ret_fd);
	}
	return (ret_fd);
}
