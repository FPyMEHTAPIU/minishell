/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msavelie <msavelie@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/21 11:06:53 by msavelie          #+#    #+#             */
/*   Updated: 2025/03/03 12:06:41 by msavelie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/minishell.h"

void	write_heredoc_line(t_heredoc *doc)
{
	if (doc->str[0] == '$')
		ft_putstr_fd(doc->expanded, doc->pipe_fd[1]);
	else
		ft_putstr_fd(doc->str, doc->pipe_fd[1]);
}

t_heredoc	init_heredoc(t_mshell *obj)
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
		obj->exit_code = 1;
		print_exit("Pipe error\n", NULL, obj);
	}
	return (heredoc_obj);
}

void	cleanup_heredoc(t_heredoc *doc)
{
	if (!doc)
		return ;
	if (doc->str)
		free(doc->str);
	doc->str = NULL;
	if (doc->trimmed)
		free(doc->trimmed);
	doc->trimmed = NULL;
	if (doc->expanded)
		free(doc->expanded);
	doc->expanded = NULL;
}

static void	count_heredocs(t_mshell *obj, t_ast_node *node)
{
	t_ast_node	*temp;
	int			i;

	temp = node;
	while (temp)
	{
		i = 0;
		while ((temp->redirs && temp->redirs[i])
			|| (temp->left && temp->left->redirs && temp->left->redirs[i]))
		{
			if ((temp->redirs && temp->redirs[i]
					&& temp->redirs[i]->type == TOKEN_HEREDOC)
				|| (temp->left && temp->left->redirs && temp->left->redirs[i]
					&& temp->left->redirs[i]->type == TOKEN_HEREDOC))
				obj->heredocs_count++;
			i++;
		}
		temp = temp->right;
	}
}

void	alloc_run_heredoc(t_mshell *obj, t_ast_node *node)
{
	t_ast_node	*temp;

	if (!obj || !node)
		return ;
	obj->heredocs_count = 0;
	obj->current_heredoc = 0;
	count_heredocs(obj, node);
	if (check_heredoc_count(obj) == 0)
		return ;
	obj->heredoc_fds = ft_calloc(obj->heredocs_count, sizeof(int));
	if (!obj->heredoc_fds)
		print_exit("Malloc error\n", NULL, obj);
	while (obj->current_heredoc < obj->heredocs_count)
	{
		obj->heredoc_fds[obj->current_heredoc] = -1;
		obj->current_heredoc++;
	}
	obj->current_heredoc = 0;
	temp = node;
	while (temp && obj->heredoc_interrupted == 0)
	{
		choose_heredoc_cmd(obj, temp);
		temp = temp->right;
	}
}
