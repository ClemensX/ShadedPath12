#include "stdafx.h"

void Billboard::init() {

}

BillboardElement & Billboard::get(string texture_id, int order_num) {
	auto &v = billboards[texture_id];
	return v.at(order_num);
}

size_t Billboard::add(string texture_id, BillboardElement billboardEl) {
	auto &v = billboards[texture_id];
	v.push_back(billboardEl);
	return v.size() - 1;
}