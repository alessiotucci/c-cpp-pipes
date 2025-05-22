/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: atucci <atucci@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/22 14:48:01 by atucci            #+#    #+#             */
/*   Updated: 2025/05/22 14:56:54 by atucci           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <vector>
#include <sys/types.h>

class Cgi
{
	public:
		Cgi();
		~Cgi();
		Cgi(const Cgi& src);
		Cgi& operator=(const Cgi& src);

		std::string execute(const std::string& scriptPath);
	private:
		char** envp_;
		// Helper to fork/pipe/exec and capture output
		std::string runForkExec(const char* script);
};
#endif
