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
#include <memory>
#include <queue>
#include <map>

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


class PakManager {
public:

	PakManager(const std::string &rootPath,unsigned int pakCacheCnt,unsigned int clusterCacheCnt);

	template<typename T>
	using RLUItem = std::pair<bool, T>;

	using PCluster = RLUItem<std::shared_ptr<Cluster> >;
	using PPakFile = RLUItem<std::shared_ptr<PakFile> >;
	class Data: public BinaryView {
	public:

		Data(const BinaryView &dataview, const std::shared_ptr<Cluster> &owner):BinaryView(dataview),owner(owner) {}
		Data():owner(nullptr) {}

		bool is_valid() const;

	protected:
		const std::shared_ptr<Cluster> owner;

	};


	Data load(const std::string &pakName, const StrViewA &fname);


	using PakMap = std::map<std::string, PPakFile>;
	using ClusterID = std::pair<std::uintptr_t, std::uint64_t>;
	using ClusterMap = std::map<ClusterID, PCluster>;


	protected:

	using PakLRU = std::queue<const std::string *>;
	using ClusterLRU = std::queue<const ClusterID *>;

	std::string rootPath;
	unsigned int pakCacheCnt;
	unsigned int clusterCacheCnt;

	PakMap pakMap;
	ClusterMap clusterMap;
	PakLRU pak_lru;
	ClusterLRU cluster_lru;

	PakMap::iterator loadPak(const std::string &name);
	ClusterMap::iterator loadCluster(PakFile &pak, const FDirItem &entry, const ClusterID &id);

};



}







#endif /* SRC_ZWEBPAK_ZWEBPAK_H_ */
