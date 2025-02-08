/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redirection.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rkhakimu <rkhakimu@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/03 15:41:46 by msavelie          #+#    #+#             */
/*   Updated: 2025/02/07 18:11:38 by rkhakimu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/minishell.h"

typedef struct s_heredoc
{
    char    *str;
    char    *trimmed;
    char    *expanded;
    t_mshell *obj;
}   t_heredoc;

static void    cleanup_heredoc(t_heredoc *doc)
{
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

static int    process_heredoc_line(t_heredoc *doc)
{
    doc->str = readline("> ");
    if (!doc->str || g_signal_received == SIGINT)
        return (0);
    doc->expanded = expand_env_vars(doc->str, doc->obj);
    doc->trimmed = ft_strtrim(doc->expanded, "\n");
    return (1);
}

static void    write_heredoc_line(t_heredoc *doc)
{
    if (doc->str[0] == '$')
        ft_fprintf(doc->obj->fd_in, "%s\n", doc->expanded);
        //ft_putstr_fd(doc->expanded, doc->obj->fd_in);
    else
        ft_fprintf(doc->obj->fd_in, "%s\n", doc->str);
        //ft_putstr_fd(doc->str, doc->obj->fd_in);
}

static int has_input_redirection(t_ast_node *cmd)
{
    int i = 0;
    if (!cmd->redirs)
        return (0);
    while (cmd->redirs[i])
    {
        if (cmd->redirs[i]->type == TOKEN_REDIRECT_IN ||
            cmd->redirs[i]->type == TOKEN_HEREDOC)
            return (1);
        i++;
    }
    return (0);
}

/*
 * A helper to check if a command node declares an output redirection
 * (either TOKEN_REDIRECT_OUT or TOKEN_REDIRECT_APPEND)
 */
static int has_output_redirection(t_ast_node *cmd)
{
    int i = 0;
    if (!cmd->redirs)
        return (0);
    while (cmd->redirs[i])
    {
        if (cmd->redirs[i]->type == TOKEN_REDIRECT_OUT ||
            cmd->redirs[i]->type == TOKEN_REDIRECT_APPEND)
            return (1);
        i++;
    }
    return (0);
}


void    handle_here_doc(t_mshell *obj, t_ast_node *node)
{
    t_heredoc    doc;

    if (node->type != TOKEN_HEREDOC)
        return;
    obj->fd_in = open(".heredoc_temp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (obj->fd_in < 0)
    {
        clean_mshell(obj);
        error_ret(6, NULL);
    }
    doc = (t_heredoc){NULL, NULL, NULL, obj};
    obj->is_heredoc = 1;
    while (process_heredoc_line(&doc))
    {
        if (!ft_strcmp(node->args[0], doc.trimmed) || 
            !ft_strcmp(node->args[0], doc.expanded))
            break;
        write_heredoc_line(&doc);
        cleanup_heredoc(&doc);
    }
    cleanup_heredoc(&doc);
    close(obj->fd_in);
}

void	redirection_input(t_mshell *obj, t_ast_node *node)
{
	if (node->type == TOKEN_REDIRECT_IN)
	{
		if (access(node->args[0], F_OK) != 0)
			exit_child(obj, node->args[0], 1, 0);
		obj->fd_in = open(node->args[0], O_RDONLY);
		if (obj->fd_in == -1)
			exit_child(obj, node->args[0], 1, 0);
	}
	else
	{
		obj->fd_in = open(".heredoc_temp", O_RDONLY);
		if (obj->fd_in == -1)
			exit_child(obj, ".heredoc_temp", 1, 0);
	}
	dup2(obj->fd_in, STDIN_FILENO);
	close(obj->fd_in);
    obj->fd_in = -1;
}

void	redirection_output(t_mshell *obj, t_ast_node *node)
{
	if (node->type == TOKEN_REDIRECT_APPEND)
		obj->fd_out = open(node->args[0],
				O_WRONLY | O_CREAT | O_APPEND, 0644);
	else
		obj->fd_out = open(node->args[0],
				O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (obj->fd_out == -1)
		exit_child(obj, node->args[0], 1, 0);
	dup2(obj->fd_out, STDOUT_FILENO);
	close(obj->fd_out);
    obj->fd_out = -1;
}

void pipe_redirection(t_mshell *obj, t_ast_node *cmd)
{
    if (obj->cur_pid == 0)
    {
        if (!has_output_redirection(cmd))
        {
            dup2(obj->pipfd[obj->cur_pid][1], STDOUT_FILENO);
            close(obj->pipfd[obj->cur_pid][1]);
            obj->pipfd[obj->cur_pid][1] = -1;
        }
    }
    else if (obj->cur_pid == obj->allocated_pipes)
    {
        if (!has_input_redirection(cmd))
        {
            dup2(obj->pipfd[obj->cur_pid - 1][0], STDIN_FILENO);
            close(obj->pipfd[obj->cur_pid - 1][0]);
            obj->pipfd[obj->cur_pid - 1][0] = -1;
        }
    }
    else
    {
        if (!has_input_redirection(cmd))
        {
            dup2(obj->pipfd[obj->cur_pid - 1][0], STDIN_FILENO);
            close(obj->pipfd[obj->cur_pid - 1][0]);
            obj->pipfd[obj->cur_pid - 1][0] = -1;
        }
        if (!has_output_redirection(cmd))
        {
            dup2(obj->pipfd[obj->cur_pid][1], STDOUT_FILENO);
            close(obj->pipfd[obj->cur_pid][1]);
            obj->pipfd[obj->cur_pid][1] = -1;
        }
    }
}