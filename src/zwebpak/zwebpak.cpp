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

std::vector<FInfo> initDirectory(const StringView<StrViewA> &files, const StrViewA &rootDir) {

	std::vector<FInfo> result;
	result.reserve(files.length);
	std::string buffer(rootDir.begin(), rootDir.end());
	std::size_t blen(buffer.length());

	for(auto ff : files) {

		buffer.append(ff.data, ff.length);
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

	std::uint32_t append_file(const StrViewA &x) {

		std::uint64_t ret = data.size();
		path.resize(plen);
		path.append(x.data, x.length);
		std::ifstream inf(path, std::ios::in| std::ios::binary);
		int i;
		while (!inf && (i = inf.get()) != EOF ) {
			data.push_back((char)i);
		}
		return static_cast<std::uint32_t>(ret);
	}
	bool isfull() const {
		return data.size() >= base_size;
	}
	void flush(std::ostream &output) {


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
	}

protected:
	std::size_t base_size;
	std::vector<char> data;
	std::string path;
	std::size_t plen;


};


bool packFiles(const StringView<StrViewA> &files, const StrViewA &rootDir, const StrViewA &targetFile, std::size_t clusterSize) {

	std::vector<FInfo > fwrk(initDirectory(files,rootDir));
	std::sort(fwrk.begin(), fwrk.end(), [](const FInfo &a, const FInfo &b) {

		StrViewA exta = get_extension(a.name);
		StrViewA extb = get_extension(b.name);
		int cmp = exta.compare(extb);
		if (cmp) {return cmp < 0;}
		return a.size < b.size;
	});

	std::string tfname(targetFile.data, targetFile.length);
	std::ofstream of(tfname, std::ios::out|std::ios::trunc|std::ios::binary);
	if (!of) return false;

	std::uint32_t entries = static_cast<std::uint32_t>(fwrk.size());
	stream_write(of, entries);

	for (auto ff :fwrk) {
		const FDirItem &itm = ff;
		stream_write(of,itm);
	}

	Clusterizer cls(clusterSize, rootDir);

	for (auto ff : fwrk) {

		ff.cluster = of.tellp();
		ff.offset = cls.append_file(ff.name);
		if (cls.isfull()) {
			cls.flush(of);
			if (!of) return false;
		}
	}

	std::sort(fwrk.begin(), fwrk.end(), compare_by_hash);

	of.seekp(4,std::ios::beg);

	for (auto ff :fwrk) {
		const FDirItem &itm = ff;
		stream_write(of, itm);
	}

	return true;
}

PakFile::PakFile(const std::string& fname):f(fname, std::ios::in|std::ios::binary) {
	if (!f) return;
	std::uint32_t cnt(0);
	stream_read(f,cnt);
	fdir.resize(cnt);
	f.read(reinterpret_cast<char *>(fdir.data()), cnt * sizeof(FDirItem));
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

}
