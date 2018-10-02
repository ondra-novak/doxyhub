/*
 * access.h
 *
 *  Created on: Jun 23, 2018
 *      Author: ondra
 */

#ifndef SRC_ZWEBPAK_ZWEBPAK_H_
#define SRC_ZWEBPAK_ZWEBPAK_H_

#include <shared/refcnt.h>
#include <shared/stringview.h>
#include <queue>
#include <unordered_map>

namespace zwebpak {

using ondra_shared::BinaryView;
using ondra_shared::StrViewA;
using ondra_shared::StringView;


bool packFiles(const StringView<StrViewA> &files, const StrViewA &rootDir, const StrViewA &targetFile, std::size_t clusterSize);

using Cluster = std::vector<unsigned char>;

struct FDirItem {
	std::uint64_t name_hash;
	std::uint64_t cluster;
	std::uint32_t offset;
	std::uint32_t size;
};


class PakFile {
public:

	PakFile(const std::string &fname);

	const FDirItem *find(const StrViewA &fname);
	Cluster load(const FDirItem &entry);
	static BinaryView extract(const Cluster &cluster, const FDirItem &entry);

	bool is_valid() const;

protected:
	std::ifstream f;
	std::vector<FDirItem> fdir;



};




}







#endif /* SRC_ZWEBPAK_ZWEBPAK_H_ */
