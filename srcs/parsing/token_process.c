/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   token_process.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msavelie <msavelie@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/03 11:31:38 by rkhakimu          #+#    #+#             */
/*   Updated: 2025/02/11 15:53:42 by msavelie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/minishell.h"

static t_token_type get_operator_type(const char *input, int *i)
{
	if (input[*i] == '|')
		return (TOKEN_PIPE);
	if (input[*i] == '<')
	{
		if (input[*i + 1] == '<')
		{
			(*i)++;
			return (TOKEN_HEREDOC);
		}
		return (TOKEN_REDIRECT_IN);
	}
	if (input[*i] == '>')
	{
		if (input[*i + 1] == '>')
		{
			(*i)++;
			return (TOKEN_REDIRECT_APPEND);
		}
		return (TOKEN_REDIRECT_OUT);
	}
	return (TOKEN_WORD);
}

t_token *handle_operator(t_token **head, t_token **current, const char *input, int *i)
{
	t_token *token;
	t_token_type type;
	int start;

	start = *i;
	type = get_operator_type(input, i);
	token = new_token(type, input + start, *i - start + 1, (*head)->mshell);
	if (!token)
		return (NULL);
	(*i)++;
	token->mshell = (*current)->mshell;
	link_token(head, current, token);
	
	// Skip spaces but continue processing if there's more content
	while (input[*i] && ft_isspace(input[*i]))
		(*i)++;
	
	return (token);
}

char	*handle_backslash(char *str)
{
	char	*new_str;
	size_t	i;
	size_t	j;
	size_t	len;
	size_t	skip_slash;

	if (!str)
		return (NULL);
	len = ft_strlen(str);
	new_str = ft_calloc(len + 1, sizeof(char));
	if (!new_str)
		return (NULL);
	i = 0;
	j = 0;
	while (i < len)
	{
		skip_slash = 0;
		if (str[i] == '\\')
			i++;
		new_str[j] = str[i];
		i++;
		j++;
	}
	new_str[j] = '\0';
	return (new_str);
}

t_token *handle_word(t_token **head, t_token **current, const char *input, int *i)
{
	t_token	*token;
	char	*expanded;
	char	*temp;
	char	*without_backslashes;
	int		start;
	
	if (!*current)
		return (NULL);
	start = *i;
	while (input[*i] && !ft_isspace(input[*i]) && 
		   !is_operator(input[*i]) && !is_quote(input[*i]))
		(*i)++;
	if (*i - start == 1 && is_quote(input[*i]))
		start++;
	temp = ft_substr(input, start, (*i) - start);
	if (!temp)
		return (NULL);
	without_backslashes = handle_backslash(temp);
	if (!without_backslashes)
	{
		free(temp);
		return (NULL);
	}
	free (temp);
	// Check if previous token exists and has quote state
	if (*current && (*current)->quote_state != QUOTE_NONE)
	{
		expanded = expand_env_vars(without_backslashes, (*current)->mshell);
		free(without_backslashes);
		if (!expanded)
			return (NULL);
		char *joined = ft_strjoin((*current)->content, expanded);
		free(expanded);
		if (!joined)
			return (NULL);
		free((*current)->content);
		(*current)->content = joined;
		return (*current);
	}
	
	expanded = expand_env_vars(without_backslashes, (*current)->mshell);
	free(without_backslashes);
	if (!expanded)
		return (NULL);
	token = new_token(TOKEN_WORD, expanded, ft_strlen(expanded), (*head)->mshell);
	free(expanded);
	if (!token)
		return (NULL);
	token->mshell = (*current)->mshell;
	link_token(head, current, token);
	return (token);
}

t_token *process_token(t_token **head, t_token **current, const char *input, int *i)
{

	if (ft_isspace(input[*i]))
	{
		(*i)++;
		return (*current);
	}
	if (is_quote(input[*i]))
		return (handle_quotes(head, current, input, i));
	if (is_operator(input[*i]))
		return (handle_operator(head, current, input, i));
	return (handle_word(head, current, input, i));
}
