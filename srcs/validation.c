/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   validation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msavelie <msavelie@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/27 14:06:50 by msavelie          #+#    #+#             */
/*   Updated: 2025/02/01 16:02:13 by msavelie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/minishell.h"

static void	free_path(char *path)
{
	if (!path || !*path)
		return ;
	free(path);
	path = NULL;
}

void	print_exit(char *mes, char *cmd, int exit_code)
{
	char	*full_msg;
	int		mes_len;

	mes_len = ft_strlen(mes) + ft_strlen(cmd) + 3;
	full_msg = ft_calloc(mes_len, sizeof(char));
	if (!full_msg)
	{
		ft_putstr_fd("Malloc failed\n", 2);
		exit (exit_code);
	}
	if (cmd)
		ft_strlcpy(full_msg, cmd, mes_len);
	ft_strlcat(full_msg, ": ", mes_len);
	ft_strlcat(full_msg, mes, mes_len);
	ft_putstr_fd(full_msg, 2);
	free (full_msg);
	exit (exit_code);
}

static void	check_is_dir(char *arg, t_mshell *obj)
{
	int	fd;

	fd = open(arg, O_DIRECTORY);
	if (fd >= 0)
	{
		close(fd);
		clean_mshell(obj);
		obj->exit_code = 126;
		print_exit("Is a directory\n", arg, obj->exit_code);
	}
}

static char	*check_paths(char **paths, char **args, size_t *args_move)
{
	int		i;
	char	*path;
	size_t	path_len;

	if (!args || (!args[0][0] && !*(args + 1)[0]))
		return (ft_strdup(""));
	else if (!args[0][0] && *(args + 2)[0])
		*args_move = 1;
	if (!paths || !*paths)
		return (ft_strdup(args[0]));
	i = 0;
	while (paths[i])
	{
		path_len = ft_strlen(paths[i]) + ft_strlen(args[0 + *args_move]) + 2;
		path = ft_calloc(path_len, sizeof(char));
		if (!path)
		{
			ft_putstr_fd("Malloc failed\n", 2);
			return (NULL);
		}
		ft_strlcpy(path, paths[i], path_len);
		ft_strlcat(path, "/", path_len);
		ft_strlcat(path, args[0 + *args_move], path_len);
		if (access(path, F_OK) == 0)
			return (path);
		free_path(path);
		i++;
	}
	return (NULL);
}

static char	**check_args(char **args)
{
	static char	*new_args[] = {"", NULL};

	if (!args)
		return (new_args);
	return (args);
}

char	*check_paths_access(char **paths, t_ast_node *node, t_mshell *obj)
{
	char	*path;

	node->args = check_args(node->args);
	check_is_dir(node->args[0], obj);
	if (node->args[0][0] == '/' || node->args[0][0] == '.')
		return (ft_strdup(node->args[0]));
	path = check_paths(paths, node->args, &obj->args_move);
	if (path && !*path)
	{
		free(path);
		exit_child(obj, NULL, 0, 0);
	}
	else if (!path || node->args[0 + obj->args_move][0] == '\0')
	{
		clean_mshell(obj);
		if (path)
			free(path);
		obj->exit_code = 127;
		print_exit("command not found\n", node->args[0 + obj->args_move], obj->exit_code);
	}
	//check_is_dir(path, obj);
	return (path);
}
