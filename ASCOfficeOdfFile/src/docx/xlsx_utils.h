#pragma once

#include <string>
#include <boost/cstdint.hpp>

#include "xlsx_cell_format.h"

namespace cpdoccore {

namespace oox {

std::wstring getColAddress(size_t col);
std::wstring getRowAddress(size_t row);

size_t getColAddressInv(const std::wstring & a);
size_t getRowAdderssInv(const std::wstring & a);

std::wstring getCellAddress(size_t col, size_t row);
void getCellAddressInv(const std::wstring & a, size_t & col, size_t & row);

bool parseBoolVal(const std::wstring & str);


std::wstring cellType2Str(XlsxCellType::type type);



bool parseDate(const std::wstring & Date, int & Year, int & Month, int & Day);
bool parseTime(const std::wstring & Time, int & Hours, int & Minutes, double & seconds);
boost::int64_t convertDate(int Year, int Month, int Day);
double convertTime(int hours, int minutes, double sec);


}
}

