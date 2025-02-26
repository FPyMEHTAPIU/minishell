/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd_utils.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msavelie <msavelie@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/24 15:04:22 by msavelie          #+#    #+#             */
/*   Updated: 2025/02/26 12:09:34 by msavelie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/minishell.h"

void	handle_empty_cd(t_mshell *obj, char *buf)
{
	char	*home_path;

	obj->exit_code = 0;
	home_path = get_env_var(obj->envp, "HOME=");
	if (!home_path)
	{
		obj->exit_code = 1;
		ft_fprintf(STDERR_FILENO, "minishell: cd: HOME not set\n");
		return ;
	}
	if (obj->prev_path)
		free(obj->prev_path);
	obj->prev_path = ft_strdup(buf);
	update_pwd(obj, buf, "OLDPWD=");
	chdir(home_path);
	update_pwd(obj, home_path, "PWD=");
	if (home_path)
		free(home_path);
}

int	handle_prev_path(t_mshell *obj)
{
	if (obj->prev_path)
		free(obj->prev_path);
	obj->prev_path = get_env_var(obj->envp, "OLDPWD=");
	if (!obj->prev_path)
	{
		obj->exit_code = 1;
		ft_fprintf(2, "minishell: cd: OLDPWD not set\n");
		return (0);
	}
	if (chdir(obj->prev_path) == -1)
	{
		obj->exit_code = 1;
		ft_fprintf(2, "minishell: cd: %s: No such file or directory\n",
			obj->prev_path);
		return (0);
	}
	printf("%s\n", obj->prev_path);
	return (1);
}
