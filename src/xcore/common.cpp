#include <string>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <QString>

static const char* hexhalf = "0123456789ABCDEF";
// static char hexbuf[5] = {'0','0','0','0',0x00};

QString formbufword(int num) {
	QString res;
	while (num) {
		res.prepend(hexhalf[num & 0x0f]);
		num >>= 4;
	}
	return res;
}

QString gethexint(int num) {
	QString res = formbufword(num);
	return res.rightJustified(8, '0');
}

QString gethex6(int num) {
	QString res = formbufword(num & 0xffffff);
	return res.rightJustified(6, '0');
}

QString gethexword(int num) {
	QString res = formbufword(num & 0xffff);
	return res.rightJustified(4, '0');
}

QString gethexbyte(int num) {
	QString res = formbufword(num & 0xff);
	return res.rightJustified(2, '0');
}

QString gethexshift(char shft) {
	QString str = (shft < 0) ? "-" : "+";
	if (shft < 0)
		shft = 256 - shft;
	str.append(gethexbyte(shft & 0x7f));
	return str;
}

QString getoctword(int num) {
	QString str;
	if (!num) {
		str = "0";
	} else {
		while(num) {
			str.prepend(hexhalf[num & 7]);
			num >>= 3;
		}
	}
	return str;
}

QString getdecshift(char shft) {
	QString str = (shft < 0) ? "-" : "+";
	if (shft < 0)
		shft = 256 - shft;
	str.append(QString::number(shft & 0x7f));
	return str;
}

QString getbinbyte(uchar num) {
	return QString::number(num+0x100,2).right(8).toUpper();
}

std::string int2str(int num) {
	std::stringstream str;
	str<<num;
	return str.str();
}

std::string float2str(float num) {
	std::stringstream str;
	str<<num;
	return str.str();
}

int toPower(int src) {
	int dst = 1;
	while (dst < src)
		dst <<= 1;
	return dst;
}

int toLimits(int src, int min, int max) {
	if (src < min) return min;
	if (src > max) return max;
	return src;
}

int getRanged(const char* str, int min, int max) {
	int res = atoi(str);
	return toLimits(res, min, max);
}

double absd(double v) {
	return (v < 0) ? -v : v;
}

std::string getTimeString(int tsec) {
	int tmin = tsec / 60;
	tsec -= tmin * 60;
	std::string res(int2str(tmin));
	res += ":";
	if (tsec < 10) res += "0";
	res += int2str(tsec);
	return res;
}

void setFlagBit(bool cond, int* val, int mask) {
	if (cond) {
		*val |= mask;
	} else {
		*val &= ~mask;
	}
}

bool str2bool(std::string v) {
	return !(v=="n" || v=="N" || v=="0" || v=="no" || v=="NO" || v=="false" || v=="FALSE" || v=="off" || v=="OFF");
}

std::vector<std::string> splitstr(std::string str,const char* spl) {
	size_t pos;
	std::vector<std::string> res;
	pos = str.find_first_of(spl);
	while (pos != std::string::npos) {
		res.push_back(str.substr(0,pos));
		str = str.substr(pos+1);
		pos = str.find_first_of(spl);
	}
	res.push_back(str);
	return res;

}

void ltrim(std::string& str) {
	size_t pos;
	pos = str.find_first_not_of(' ');
	str.erase(0,pos);
}

void rtrim(std::string& str) {
	size_t pos;
	pos = str.find_last_not_of(' ');
	if (pos == std::string::npos) return;
	str.erase(pos+1,std::string::npos);
}

void trim(std::string& str) {
	ltrim(str);
	rtrim(str);
}

std::pair<std::string,std::string> splitline(std::string line, char delim) {
	size_t pos;
	std::pair<std::string,std::string> res;
	do {pos = line.find("\r"); if (pos!=std::string::npos) line.erase(pos);} while (pos!=std::string::npos);
	do {pos = line.find("\n"); if (pos!=std::string::npos) line.erase(pos);} while (pos!=std::string::npos);
	res.first = "";
	res.second = "";
	pos = line.find(delim);
	if (pos!=std::string::npos) {
		res.first = std::string(line,0,pos);
		res.second = std::string(line,pos+1);
	} else {
		res.first = line;
		res.second = "";
	}
	trim(res.first);
	trim(res.second);
	return res;
}
