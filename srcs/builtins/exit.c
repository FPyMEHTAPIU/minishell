/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exit.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msavelie <msavelie@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/17 18:46:04 by msavelie          #+#    #+#             */
/*   Updated: 2025/02/17 18:46:34 by msavelie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/minishell.h"

static void	handle_wrong_exit_code(t_mshell *obj, char *arg, char *msg, long exit_code)
{
	obj->exit_code = exit_code;
	if (arg)
		ft_fprintf(2, "minishell: exit: %s%s\n", arg, msg);
	else
		ft_fprintf(2, "minishell: exit: %s\n", msg);
	clean_mshell(obj);
	free(obj->envp);
	exit(obj->exit_code);
}

static void	check_exit_code(char *arg, t_mshell *obj, int args_len)
{
	int	i;
	int	sign;

	if (!*arg)
		handle_wrong_exit_code(obj, arg, ": numeric argument required", 2);
	i = 0;
	sign = 0;
	while (arg[i])
	{
		if ((sign == 1 && (arg[i] == '-' || arg[i] == '+'))
			|| (ft_isdigit(arg[i]) == 0 && arg[i] != '-' && arg[i] != '+'))
			handle_wrong_exit_code(obj, arg, ": numeric argument required", 2);
		if (arg[i] == '-' || arg[i] == '+')
			sign = 1;
		i++;
	}
	if (args_len > 2)
		handle_wrong_exit_code(obj, NULL, "too many arguments", 1);
	obj->exit_code = ft_atol(arg);
}

void	check_and_handle_exit(char **args, t_mshell *obj)
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
		check_exit_code(args[1], obj, args_len);
		clean_mshell(obj);
		//free(obj->envp);
		exit(obj->exit_code);
	}
}
