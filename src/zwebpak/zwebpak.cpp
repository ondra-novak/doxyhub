/*
 * zwebpak.cpp
 *
 *  Created on: Jun 23, 2018
 *      Author: ondra
 */




#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <zlib.h>
#include "zwebpak.h"
#include <imtjson/fnv.h>

namespace zwebpak {

static StrViewA get_extension(StrViewA z) {
	auto pos = z.lastIndexOf(".");
	if (pos == z.npos) {
		return StrViewA();
	} else {
		return z.substr(pos);
	}
}


struct FInfo: FDirItem {
	StrViewA name;
};


static std::uint64_t calc_hash(StrViewA name) {

	std::uint64_t v = 0;
	FNV1a64 fnv(v);
	for (auto x : name) {
		fnv(x);
	}
	return v;

}

bool compare_by_hash(const FDirItem &a, const FDirItem &b) {
	return a.name_hash < b.name_hash;
}

std::vector<FInfo> initDirectory(const StringView<std::string> &files, const std::string &rootDir) {

	std::vector<FInfo> result;
	result.reserve(files.length);
	std::string buffer(rootDir.begin(), rootDir.end());
	std::size_t blen(buffer.length());

	for(auto &&ff : files) {

		buffer.append(ff);
		struct stat s;
		if (stat(buffer.c_str(), &s) == 0) {

			FInfo r;
			r.name = ff;
			r.size = static_cast<std::uint32_t>(s.st_size);
			r.name_hash = calc_hash(ff);
			result.push_back(r);
		}
		buffer.resize(blen);
	}

	return result;
}

template<typename S, typename T>
void stream_write(S &s, const T &t) {
	s.write(reinterpret_cast<const char *>(&t), sizeof(T));
}
template<typename S, typename T>
void stream_read(S &s, T &t) {
	s.read(reinterpret_cast<char *>(&t), sizeof(T));
}


class Clusterizer {
public:

	Clusterizer(std::size_t base_size, const StrViewA &rootDir)
		:base_size(base_size)
		,path(rootDir.begin(), rootDir.end())
		,plen(rootDir.length) {}

	void append_file(FInfo &ff) {

		ff.offset = data.size();
		path.resize(plen);
		path.append(ff.name.data, ff.name.length);
		std::ifstream inf(path, std::ios::in| std::ios::binary);
		int i;

		std::uint64_t content_hash;
		FNV1a64 hash_calc(content_hash);

		while (!(!inf) && (i = inf.get()) != EOF ) {
			char c = static_cast<char>(i);
			data.push_back(c);
			hash_calc(c);
		}

		ff.content_hash = content_hash;
	}
	bool isfull() const {
		return data.size() >= base_size;
	}
	void flush(std::ostream &output) {

		if (data.empty()) return;

		std::vector<char> obuff;
		obuff.resize(compressBound(data.size()));

		uLongf csz = obuff.size();

		compress2(reinterpret_cast<Bytef *>(obuff.data()), &csz,
					reinterpret_cast<const Bytef *>(data.data()), data.size(),9);

		std::uint32_t cmpsz;
		std::uint32_t orgsz;
		StrViewA toWrite;

		if (csz > data.size()) {

			cmpsz = 0;
			orgsz = static_cast<std::uint32_t>(data.size());
			toWrite = StrViewA(data.data(), data.size());

		} else {
			cmpsz = static_cast<std::uint32_t>(csz);
			orgsz = static_cast<std::uint32_t>(data.size());
			toWrite = StrViewA(obuff.data(), csz);
		}

		stream_write(output, cmpsz);
		stream_write(output, orgsz);

		output.write(toWrite.data, toWrite.length);
		data.resize(0);
	}

protected:
	std::size_t base_size;
	std::vector<char> data;
	std::string path;
	std::size_t plen;


};


bool packFiles(const StringView<std::string> &files,
		const std::string &rootDir,
		const std::string &targetFile,
		const std::string &revision,
		std::size_t clusterSize) {

	std::string shortrev = revision.substr(0,revision_size);

	std::vector<FInfo > fwrk(initDirectory(files,rootDir));
	std::sort(fwrk.begin(), fwrk.end(), [](const FInfo &a, const FInfo &b) {

		StrViewA exta = get_extension(a.name);
		StrViewA extb = get_extension(b.name);
		int cmp = exta.compare(extb);
		if (cmp) {return cmp < 0;}
		return a.size < b.size;
	});

	std::ofstream of(targetFile, std::ios::out|std::ios::trunc|std::ios::binary);
	if (!of) return false;

	std::uint32_t entries = static_cast<std::uint32_t>(fwrk.size());

	PakHeader hdr;
	std::copy(pak_magic.begin(), pak_magic.end(), std::begin(hdr.magic));
	std::copy(shortrev.begin(), shortrev.end(), std::begin(hdr.rev));
	hdr.dir_size = entries;
	hdr.version = pak_version;

	stream_write(of, hdr);

	for (auto ff :fwrk) {
		const FDirItem &itm = ff;
		stream_write(of,itm);
	}

	Clusterizer cls(clusterSize, rootDir);

	for (auto &&ff : fwrk) {

		ff.cluster = of.tellp();
		cls.append_file(ff);
		if (cls.isfull()) {
			cls.flush(of);
			if (!of) return false;
		}
	}
	cls.flush(of);
	if (!of) return false;

	std::sort(fwrk.begin(), fwrk.end(), compare_by_hash);

	of.seekp(sizeof(PakHeader),std::ios::beg);

	for (auto ff :fwrk) {
		const FDirItem &itm = ff;
		stream_write(of, itm);
	}

	return true;
}

PakFile::PakFile(const std::string& fname):f(fname, std::ios::in|std::ios::binary) {
	if (!f) return;
	PakHeader hdr;
	stream_read(f,hdr);
	if (StrViewA(hdr.magic,pak_magic.length) != pak_magic) return;
	if (hdr.version != pak_version) return;
	revision.append(hdr.rev, revision_size);

	fdir.resize(hdr.dir_size);
	f.read(reinterpret_cast<char *>(fdir.data()), hdr.dir_size * sizeof(FDirItem));
}

const FDirItem *PakFile::find(const StrViewA& fname) {

	FDirItem x;
	x.name_hash = calc_hash(fname);
	auto r = std::lower_bound(fdir.begin(), fdir.end(), x, compare_by_hash);
	if (r == fdir.end() || r->name_hash != x.name_hash) return nullptr;
	return &(*r);
}

Cluster PakFile::load(const FDirItem& entry) {
	f.seekg(entry.cluster, std::ios::beg);

	std::uint32_t cmpsz;
	std::uint32_t orgsz;
	stream_read(f, cmpsz);
	stream_read(f, orgsz);

	Cluster buf1, buf2;
	buf1.resize(orgsz);

	if (cmpsz == 0) {

		f.read(reinterpret_cast<char *>(buf1.data()),buf1.size());

	} else {
		buf2.resize(cmpsz);
		f.read(reinterpret_cast<char *>(buf2.data()),buf2.size());
		uLongf destlen = buf1.size();
		uncompress(buf1.data(),&destlen,buf2.data(),buf2.size());
		buf1.resize(destlen);
	}

	return buf1;
}

BinaryView PakFile::extract(const Cluster& cluster, const FDirItem& entry) {

	return BinaryView(cluster.data(), cluster.size()).substr(entry.offset, entry.size);

}

bool PakFile::is_valid() const {
	return !(!f);
}


PakManager::PakManager(const std::string& rootPath, unsigned int pakCacheCnt,unsigned int clusterCacheCnt)
	:rootPath(rootPath)
	,pakCacheCnt(pakCacheCnt)
	,clusterCacheCnt(clusterCacheCnt)
{



}

PakManager::Data PakManager::load(const std::string& pakName, const StrViewA& fname) {

	auto i1 = pakMap.find(pakName);
	if (i1 == pakMap.end()) i1 = loadPak(pakName);
	if (i1 == pakMap.end()) return Data();

	i1->second.used = true;

	const FDirItem *d = i1->second.object->find(fname);
	if (d == nullptr) return Data();

	ClusterKey cid(&i1->first, d->cluster);

	auto i2 = clusterMap.find(cid);
	if (i2 == clusterMap.end())
		i2 = loadCluster(*i1->second.object, *d, cid);
	if (i2 == clusterMap.end())
		return Data();

	i1->second.used= true;


	return Data(PakFile::extract(*i2->second.object, *d), i2->second.object, d->content_hash);

}

template<typename LRU, typename MAP>
void clear_cache(LRU &pak_lru, std::size_t pakCacheCnt, MAP &pakMap) {
	while (pak_lru.size() >= pakCacheCnt) {
		std::size_t cnt = 0;
		while (true) {
			auto i1 = pakMap.find(*pak_lru.front());
			if (i1 == pakMap.end() || !i1->second.used || cnt >=pakCacheCnt/2) {
				pakMap.erase(i1);
				pak_lru.pop();
				break;
			} else {
				i1->second.used = false;
				pak_lru.push(pak_lru.front());
				pak_lru.pop();
				cnt++;
			}
		}
	}

}

PakManager::PakMap::iterator PakManager::loadPak(const std::string& name) {


	PPakFile pp(nullptr);
	try {
		std::string fname = rootPath+name;
		pp = PPakFile (std::make_shared<PakFile>(fname));
		if (!pp.object->is_valid()) return pakMap.end();
	} catch (...) {
		onLoadDone();
		throw;
	}
	onLoadDone();
	clear_cache(pak_lru, pakCacheCnt, pakMap);
	auto i1 = pakMap.insert(std::make_pair(name, pp)).first;
	pak_lru.push(&i1->first);
	return i1;
}

void PakManager::invalidate(const std::string& pakName) {
	pakMap.erase(pakName);
}

bool PakManager::getContentHash(const std::string &pakName, const StrViewA &fname, std::uint64_t &hash) {
	auto i1 = pakMap.find(pakName);
	if (i1 == pakMap.end()) i1 = loadPak(pakName);
	if (i1 == pakMap.end()) return false;

	i1->second.used = true;

	const FDirItem *d = i1->second.object->find(fname);
	if (d == nullptr) return false;
	hash = d->content_hash;
	return true;
}

PakManager::ClusterMap::iterator PakManager::loadCluster( PakFile& pak,
		const FDirItem& entry, const ClusterKey& id) {


	PCluster clst(nullptr);
	try {
		clst = PCluster(std::make_shared<Cluster>(pak.load(entry)));
	} catch (...) {
		onLoadDone();
		throw;
	}
	onLoadDone();
	clear_cache(cluster_lru, clusterCacheCnt, clusterMap);
	auto i1 = clusterMap.insert(std::make_pair(std::move(id), std::move(clst))).first;
	cluster_lru.push(&i1->first);
	return i1;


}

bool PakManager::Data::is_valid() const {
	return owner != nullptr;
}

std::string PakManager::getRevision(const std::string& pakName) {
	auto i1 = pakMap.find(pakName);
	if (i1 == pakMap.end()) i1 = loadPak(pakName);
	if (i1 == pakMap.end()) return std::string();

	i1->second.used = true;
	return i1->second.object->getRevision();
}



}

