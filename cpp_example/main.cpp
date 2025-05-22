/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: atucci <atucci@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/22 14:45:55 by atucci            #+#    #+#             */
/*   Updated: 2025/05/22 14:50:36 by atucci           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
// main.cpp
#include <iostream>
#include "Cgi.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <script_path>\n";
        return 1;
    }

    try {
        Cgi cgi;  // use default environment
        std::string output = cgi.execute(argv[1]);
        std::cout << "=== CGI Output ===\n"
                  << output
                  << "--- End of Output ---\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error running CGI: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

