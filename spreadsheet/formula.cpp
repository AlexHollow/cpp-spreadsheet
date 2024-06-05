#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
	explicit Formula(std::string expression)
		try : ast_(ParseFormulaAST(expression)) {
	} catch (const FormulaException&) {
		throw;
	}
    
    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute([&sheet](const Position pos) -> double {

				if (!pos.IsValid()) {
					throw FormulaError(FormulaError::Category::Ref);
				}

				const CellInterface* cell = sheet.GetCell(pos);

				if (!cell) {
					return 0;
				}

				CellInterface::Value cell_value = cell->GetValue();

				if (std::holds_alternative<double>(cell_value)) {
					return std::get<double>(cell_value);
				}

				if (std::holds_alternative<std::string>(cell_value)) {
					std::string str = std::get<std::string>(cell_value);

					if (str.empty()) {
						return 0;
					}

					double value = 0.0;
					std::istringstream iss(str);

					if (!(iss >> value) || !iss.eof()) {
						throw FormulaError(FormulaError::Category::Value);
					}

					return value;
				}

                throw FormulaError(FormulaError::Category::Value);
		    });

        } catch (const FormulaError& ex) {
            return ex;
        }
    }
    
    std::string GetExpression() const override {
        std::ostringstream out_ss;
        ast_.PrintFormula(out_ss);
        return out_ss.str();
    }

    std::vector<Position> GetReferencedCells() const {
        std::set<Position> result;

        for (const auto& cell_pos : ast_.GetCells()) {
            if (cell_pos.IsValid()) {
                result.emplace(cell_pos);
            }
        }

        return std::vector<Position>(result.begin(), result.end());
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (std::exception&) {
        throw FormulaException(std::string());
    }
}

// -------- FormulaError --------

FormulaError::FormulaError(Category category) : category_{ category } {}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
    case FormulaError::Category::Ref:
        return "#REF!"sv;
    case FormulaError::Category::Value:
        return "#VALUE!"sv;
    case FormulaError::Category::Arithmetic:
        return "#ARITHM!"sv;
    default:
        return {};
    }
}