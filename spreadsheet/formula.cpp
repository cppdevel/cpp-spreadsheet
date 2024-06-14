#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <string_view>

using namespace std::literals;

FormulaError::FormulaError(Category category)
        : category_(category)
{
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
        case FormulaError::Category::Ref:
            return "#REF!";
        case FormulaError::Category::Value:
            return "#VALUE!";
        case FormulaError::Category::Arithmetic:
            return "#ARITHM!";
        default:
            return "";
    }
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression) try
            : ast_(ParseFormulaAST(expression)) {
        } catch (const std::exception& exc) {
            std::throw_with_nested(FormulaException(exc.what()));
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                return ast_.Execute(sheet);
            }
            catch (const FormulaError& evaluate_error) {
                return evaluate_error;
            }
        }

        std::vector<Position> GetReferencedCells() const override {
            std::set referenced_cells_set(ast_.GetCells().begin(), ast_.GetCells().end());
            return { referenced_cells_set.begin(), referenced_cells_set.end() };
        }

        std::string GetExpression() const override {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) {
        throw FormulaException("");
    }
}