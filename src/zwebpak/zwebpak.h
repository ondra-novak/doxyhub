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
#include <fstream>
#include <memory>
#include <queue>
#include <map>

namespace zwebpak {

using ondra_shared::BinaryView;
using ondra_shared::StrViewA;
using ondra_shared::StringView;


bool packFiles(const StringView<std::string> &files, const std::string &rootDir, const std::string &targetFile, std::size_t clusterSize);

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
	struct RLUItem {
		bool used;
		T object;

		RLUItem(const T &a):used(true),object(a) {}
		RLUItem(T &&a):used(true),object(std::move(a)) {}
	};
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
	void invalidate(const std::string &pakName);


	using PakMap = std::map<std::string, PPakFile>;
	using PakID = const std::string *;
	using ClusterID = std::uint64_t;
	using ClusterKey = std::pair<PakID, ClusterID>;
	using ClusterMap = std::map<ClusterKey, PCluster>;
	using ClusterKeyID =const ClusterKey *;


	protected:

	using PakLRU = std::queue<PakID>;
	using ClusterLRU = std::queue<ClusterKeyID>;

	std::string rootPath;
	unsigned int pakCacheCnt;
	unsigned int clusterCacheCnt;

	PakMap pakMap;
	ClusterMap clusterMap;
	PakLRU pak_lru;
	ClusterLRU cluster_lru;

	virtual PakMap::iterator loadPak(const std::string &name);
	virtual ClusterMap::iterator loadCluster(PakFile &pak, const FDirItem &entry, const ClusterKey &id);





};



}







#endif /* SRC_ZWEBPAK_ZWEBPAK_H_ */
