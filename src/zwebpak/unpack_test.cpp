/*
 * unpack_test.cpp
 *
 *  Created on: 6. 10. 2018
 *      Author: ondra
 */

#include <iostream>
#include "zwebpak.h"




int main(int argc, char *argv[]) {

	if (argc != 2) {

		std::cerr << "Need argument: path to pack" << std::endl;
		return 1;

	}

	zwebpak::PakManager pmanager("",16,16);
	std::string ln;

	std::string rev;
	if (!pmanager.getRevision(argv[1],rev))
	{
		std::cerr << "Archive open error" << std::endl;
		return 2;
	}
	std::cerr << "Opened revision: " << rev << std::endl;

	while (!!std::cout) {
		std::getline(std::cin, ln);
		if (!ln.empty()) {
			auto b = pmanager.load(argv[1],ln);
			if (b.is_valid()) {
				std::cerr << "FOUND: length=" << b.length << ", hash=" << b.content_hash << std::endl;
				std::cout.write(reinterpret_cast<const char *>(b.data), b.length);
				std::cerr << std::endl;
			} else {
				std::cerr << "Not found" << std::endl;
			}
		}
	}

	return 0;
}
