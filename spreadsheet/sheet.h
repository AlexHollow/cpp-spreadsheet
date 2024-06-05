#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    CellInterface* GetCell(Position pos) override;
    const CellInterface* GetCell(Position pos) const override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    void CheckPosition(Position pos) const;
    CellInterface* GetCellPtr(Position pos) const;
    void RecalculatePrintableSize() const;

    struct TableComparator {
        bool operator()(const Position& lhs, const Position& rhs) const;
    };

    struct TableHasher {
        size_t operator()(const Position& pos) const;
    };

private:
    std::unordered_map<Position, std::unique_ptr<Cell>, TableHasher, TableComparator> table_;

    // по заданию нельзя менять интерфейс;
    // пришлось прибегнуть к mutable, чтобы лишний раз не пересчитывать размер зоны печати
    mutable Size printable_size_ = { 0, 0 };
    mutable bool is_need_to_resize_ = false; // флаг - необходимо ли пересчитывать размер
};