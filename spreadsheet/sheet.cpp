#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>


using namespace std::literals;

// -------- Table Comp & Hasher --------

bool Sheet::TableComparator::operator()(const Position& lhs, const Position& rhs) const {
    return lhs == rhs;
}

size_t Sheet::TableHasher::operator()(const Position& pos) const {
	return std::hash<std::string>()(pos.ToString());
}

// -------- Sheet Class --------

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    CheckPosition(pos);

    if (table_.count(pos) == 0) {
        table_[pos] = std::make_unique<Cell>(*this);
    }

    table_[pos]->Set(std::move(text));

    if (pos.row >= printable_size_.rows || pos.col >= printable_size_.cols) {
        is_need_to_resize_ = true;
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetCellPtr(pos);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellPtr(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!GetCell(pos)) { return; }

    table_.erase(pos);

    if (printable_size_.rows == pos.row + 1 || printable_size_.cols == pos.col + 1) {
        is_need_to_resize_ = true;
    }
}

Size Sheet::GetPrintableSize() const {
    if (is_need_to_resize_) {
        RecalculatePrintableSize();
    }
    return printable_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size print_size = GetPrintableSize();

    for (int row = 0; row < print_size.rows; ++row) {
        bool is_first = true;
        for (int col = 0; col < print_size.cols; ++col) {
            if (!is_first) {
                output << '\t';
            }

            is_first = false;
            const CellInterface* cell_ptr = GetCellPtr({row, col});

            if (cell_ptr) {
                auto value = cell_ptr->GetValue();
                std::visit([&](const auto& x) { output << x; }, value);
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size print_size = GetPrintableSize();

    for (int row = 0; row < print_size.rows; ++row) {
        bool is_first = true;
        for (int col = 0; col < print_size.cols; ++col) {
            if (!is_first) {
                output << '\t';
            }

            is_first = false;
            const CellInterface* cell_ptr = GetCellPtr({ row, col });

            if (cell_ptr) {
                output << cell_ptr->GetText();
            }
        }
        output << '\n';
    }
}

void Sheet::CheckPosition(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
}

CellInterface* Sheet::GetCellPtr(Position pos) const {
    CheckPosition(pos);

    if (table_.count(pos) != 0) {
        return table_.at(pos).get();
    }

    return nullptr;
}

void Sheet::RecalculatePrintableSize() const {
    Size new_print_size = { 0, 0 };

    for (auto it = table_.begin(); it != table_.end(); ++it) {
        if (it->second != nullptr) {
            int row = it->first.row;
            int col = it->first.col;
            new_print_size.rows = std::max(new_print_size.rows, row + 1);
            new_print_size.cols = std::max(new_print_size.cols, col + 1);
        }
    }

    printable_size_ = new_print_size;
    is_need_to_resize_ = false;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}