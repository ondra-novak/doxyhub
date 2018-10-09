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


bool packFiles(const StringView<std::string> &files,
		const std::string &rootDir,
		const std::string &targetFile,
		const std::string &revision,
		std::size_t clusterSize);

using Cluster = std::vector<unsigned char>;

struct FDirItem {
	std::uint64_t name_hash;
	std::uint64_t content_hash;
	std::uint64_t cluster;
	std::uint32_t offset;
	std::uint32_t size;
};

constexpr ondra_shared::StrViewA pak_magic("ZWEBPAK\0",8);
constexpr std::uint32_t pak_version(0x100);
constexpr int revision_size = 8;

struct PakHeader {
	char magic[pak_magic.length];
	char rev[revision_size];
	std::uint32_t version;
	std::uint32_t dir_size;
};

class PakFile {
public:

	PakFile(const std::string &fname);

	const FDirItem *find(const StrViewA &fname);
	Cluster load(const FDirItem &entry);
	static BinaryView extract(const Cluster &cluster, const FDirItem &entry);
	const std::string &getRevision() const {return revision;}

	bool is_valid() const;

protected:
	std::ifstream f;
	std::vector<FDirItem> fdir;
	std::string revision;

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

		Data(const BinaryView &dataview,
			const std::shared_ptr<Cluster> &owner,
			std::uint64_t content_hash)
			:BinaryView(dataview),content_hash(content_hash),owner(owner) {}

		Data():content_hash(0),owner(nullptr) {}

		bool is_valid() const;

		const std::uint64_t content_hash;
	protected:
		const std::shared_ptr<Cluster> owner;


	};


	///Loads data from the pak file
	/**
	 * @param pakName name of zwebpak file
	 * @param fname name (or path) of packed file inside of the zwebpak file
	 * @return loaded data. Always check is_valid() which is returns false, when
	 * the file cannot be loaded
	 */
	Data load(const std::string &pakName, const StrViewA &fname);

	///Determines content_hash of the file
	/** returns hash of content of the file. This only need to access
	 * directory of the pak file, without need to decode whole cluster. It
	 * is purposed to support ETags. When ETag matches, no cluster
	 * need to be decoded for the request
	 * @param pakName name of zwebpak
	 * @param fname name (or path) inside of zwebpak
	 * @param hash reference to variable which receives the hash
	 * @retval true success
	 * @retval false failure, probably file not found
	 */
	bool getContentHash(const std::string &pakName, const StrViewA &fname, std::uint64_t &hash);
	///Invalidates the pakfile
	/** this is need, when underlying file has been updated
	 *
	 * @param pakName name of zwebpak file
	 *
	 * @note this removes directory from the cache, so next request will
	 * load a new version of the directory. However, any already opened
	 * cluster is not invalidated, it must be closed first
	 */
	void invalidate(const std::string &pakName);


	///Retrieves revision string (short revision) of this webpak
	std::string getRevision(const std::string &pakName);

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
