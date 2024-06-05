#include "cell.h"

#include <cassert>
#include <iostream>
#include <stack>
#include <string>
#include <optional>
#include <unordered_set>

// -------- Additional classes --------

// -------- Base Cell --------

class Cell::Impl {
public:
	virtual ~Impl() = default;

	virtual Cell::Value GetValue() const = 0;
	virtual std::string GetText() const = 0;

	virtual void InvalidateCache() {}
	virtual std::vector<Position> GetReferencedCells() const { return {}; }

private:
};

// -------- Text Cell --------

class Cell::TextImpl final : public Cell::Impl {
public:
	TextImpl(std::string str) : text_{ std::move(str) } {}

	Cell::Value GetValue() const override {
		if (text_[0] == ESCAPE_SIGN) {
			return text_.substr(1);
		}
		return text_;
	}

	std::string GetText() const override {
		return text_;
	}

private:
	std::string text_;
};

// -------- Formula Cell --------

class Cell::FormulaImpl final : public Cell::Impl {
public:
	FormulaImpl(std::string str, SheetInterface& sheet)
		try : formula_(ParseFormula(str.substr(1))), sheet_{ sheet } {
	} catch (const FormulaException&) {
		throw;
	}

	Cell::Value GetValue() const override {
		if (!cache_) {
			cache_ = formula_->Evaluate(sheet_);
		}

		if (std::holds_alternative<double>(*cache_)) {
			return std::get<double>(*cache_);
		}
		return std::get<FormulaError>(*cache_);
	}

	std::string GetText() const override {
		return FORMULA_SIGN + formula_->GetExpression();
	}

	void InvalidateCache() override {
		cache_.reset();
	}

	std::vector<Position> GetReferencedCells() const override {
		return formula_->GetReferencedCells();
	}

private:
	std::unique_ptr<FormulaInterface> formula_ = nullptr;
	SheetInterface& sheet_;
	mutable std::optional<FormulaInterface::Value> cache_;
};

// -------- Empty Cell --------

class Cell::EmptyImpl final : public Cell::Impl {
public:
	Cell::Value GetValue() const override {
		return std::string();
	}

	std::string GetText() const override {
		return std::string();
	}

private:
};


// -------- Cell Class --------

Cell::Cell(SheetInterface& sheet) : sheet_{ sheet }, impl_{ std::make_unique<EmptyImpl>() } {}

Cell::~Cell() {}

void Cell::Set(std::string text) {
	if (text.empty()) {
		impl_ = std::make_unique<EmptyImpl>();
		return;
	}

	std::unique_ptr<Impl> temp_impl;
	
	if (text.size() > 1 && text[0] == FORMULA_SIGN) {
		temp_impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
	
	} else {
		temp_impl = std::make_unique<TextImpl>(std::move(text));
	}

	if (HasCircularDependency(*temp_impl)) {
		throw CircularDependencyException(std::string());
	}

	impl_ = std::move(temp_impl);

	for (const auto& cell_pos : impl_->GetReferencedCells()) {
		Cell* cell = reinterpret_cast<Cell*>(sheet_.GetCell(cell_pos));

		if (!cell) {
			sheet_.SetCell(cell_pos, std::string());
			cell = reinterpret_cast<Cell*>(sheet_.GetCell(cell_pos));
		}

		cell->children_.insert(this);
	}

	InvalidateCache();
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

void Cell::InvalidateCache() {
	impl_->InvalidateCache();

	for (Cell* cell : children_) {
		cell->InvalidateCache();
	}
}

bool Cell::HasCircularDependency(const Impl& impl) const {
	std::unordered_set<const Cell*> cells;
	std::stack<const Cell*> need_to_visit;
	std::unordered_set<const Cell*> visited;

	for (const auto& pos : impl.GetReferencedCells()) {
		const Cell* cell = reinterpret_cast<Cell*>(sheet_.GetCell(pos));
		cells.insert(cell);
	}

	need_to_visit.push(this);

	while (!need_to_visit.empty()) {
		const Cell* current_cell = need_to_visit.top();
		need_to_visit.pop();

		if (cells.count(current_cell)) {
			return true;
		}

		visited.insert(current_cell);

		for (const Cell* cell : current_cell->children_) {
			if (!visited.count(cell)) {
				need_to_visit.push(cell);
			}
		}
	}

	return false;
}