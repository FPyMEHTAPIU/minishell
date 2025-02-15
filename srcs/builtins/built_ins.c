/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   built_ins.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msavelie <msavelie@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/23 14:14:26 by msavelie          #+#    #+#             */
/*   Updated: 2025/02/13 15:12:55 by msavelie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/minishell.h"

void	realloc_buffer(char **buf, size_t *buffer_size)
{
	*buffer_size *= 2;
	free(*buf);
	*buf = ft_calloc(*buffer_size, sizeof(char));
	if (!*buf)
	{
		ft_fprintf(2, "Malloc error!\n");
		exit(1);
	}
}

int	cd(char **cd_args, t_mshell *obj)
{
	char	buf[PATH_BUFFER_SIZE];
	char	*path;
	char	*full_path;

	if (!cd_args[1] || !*cd_args[1])
	{
		obj->exit_code = 0;
		chdir(get_env_var(obj->envp, "HOME"));
		return (1);
	}
	else if (cd_args[2] && *cd_args[2])
	{
		obj->exit_code = 1;
		ft_fprintf(2, "minishell: cd: too many arguments\n");
		return (1);
	}
	if (!getcwd(buf, PATH_BUFFER_SIZE))
	{
		perror("getcwd");
		obj->exit_code = 1;
	}
	path = ft_strjoin(buf, "/");
	full_path = ft_strjoin(path, cd_args[1]);
	free(path);
	if (ft_strcmp(buf, cd_args[1]) == 0)
		obj->exit_code = 0;
	else if (chdir(full_path) == -1)
	{
		obj->exit_code = 1;
		ft_fprintf(2, "minishell: cd: %s: No such file or directory\n", cd_args[1]);
	}
	else
		obj->exit_code = 0;
	if (!getcwd(buf, PATH_BUFFER_SIZE))
	{
		perror("getcwd");
		obj->exit_code = 1;
	}
	free(full_path);
	return (1);
}

int	pwd(t_mshell *obj)
{
	char	buf[PATH_BUFFER_SIZE];
	char	*error_args[] = {"cd", NULL};

	if (!getcwd(buf, PATH_BUFFER_SIZE))
	{
		perror("getcwd");
		cd(error_args, obj);
	}
	else
		printf("%s\n", buf);
	obj->exit_code = 0;
	return (1);
}

void	set_env_args(t_mshell *obj, t_ast_node *node)
{
	int	i;

	i = 0;
	while (node->args[i])
		i++;
	ft_free_strs(node->args, i);
	node->args = ft_calloc(3, sizeof(char *));
	if (!node->args)
	{
		clean_mshell(obj);
		return ;
	}
	node->args[0] = ft_strdup("cat");
	node->args[1] = ft_strdup(".env_temp.txt");
	node->args[2] = NULL;
}

int	env(t_mshell *obj)
{
	size_t	i;

	if (!obj || !obj->envp)
		return (0);
	i = 0;
	while (obj->envp[i])
	{
		printf("%s\n", obj->envp[i]);
		i++;
	}
	return (1);
}

int	echo(char **args, t_mshell *obj)
{
	int	i;

	if (!args || !*args)
	{
		printf("\n");
		return (0);
	}
	i = 1;
	if (ft_strcmp(args[1], "-n") == 0)
		i++;
	while (args[i])
	{
		printf("%s", args[i]);
		if (args[i + 1])
			printf(" ");
		i++;
	}
	if (ft_strcmp(args[1], "-n") != 0)
		printf("\n");
	obj->exit_code = 0;
	return (1);
}
