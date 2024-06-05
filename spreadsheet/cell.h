#pragma once

#include "common.h"
#include "formula.h"
#include <unordered_set>


class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const;
    void InvalidateCache();

private:
    class Impl;
    class TextImpl;
    class FormulaImpl;
    class EmptyImpl;

    SheetInterface& sheet_;
    std::unique_ptr<Impl> impl_;
    std::unordered_set<Cell*> children_;

    bool HasCircularDependency(const Impl& impl) const;
};