#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <stack>
#include <string>
#include <memory>

Cell::Cell(Sheet& sheet)
        : impl_(std::make_unique<EmptyImpl>())
        , sheet_(sheet)
{
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> impl;
    if (text.empty()) {
        impl = std::make_unique<EmptyImpl>();
    }
    else if (text[0] == FORMULA_SIGN && text.size() > 1) {
        impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    }
    else {
        impl = std::make_unique<TextImpl>(std::move(text));
    }
    if (IsReferenced(*impl)) {
        throw CircularDependencyException("Circular Dependency Exception");
    }
    impl_ = std::move(impl);
    for (Cell* cell : referring_from_) {
        cell->referring_to_.erase(this);
    }
    referring_from_.clear();
    std::vector<Position> formula_cells = GetReferencedCells();
    for (const Position pos : formula_cells) {
        Cell* cell_ptr = sheet_.GetConcreteCell(pos);
        if (cell_ptr == nullptr) {
            sheet_.SetCell(pos, "");
            cell_ptr = sheet_.GetConcreteCell(pos);
        }
        referring_from_.insert(cell_ptr);
        cell_ptr->referring_to_.insert(this);
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

bool Cell::IsReferenced(const Impl& impl) const {
    if (impl.GetReferencedCells().empty()) {
        return false;
    }
    std::unordered_set<const Cell*> referenced_cells;
    for (const auto& pos : impl.GetReferencedCells()) {
        referenced_cells.insert(sheet_.GetConcreteCell(pos));
    }
    std::unordered_set<const Cell*> visited_cells;
    std::stack<const Cell*> cells_to_visit;
    cells_to_visit.push(this);
    while (!cells_to_visit.empty()) {
        const Cell* current_cell = cells_to_visit.top();
        cells_to_visit.pop();
        visited_cells.insert(current_cell);
        if (referenced_cells.find(current_cell) != referenced_cells.end()){
            return true;
        }
        for (const Cell* c : current_cell->referring_to_) {
            if (visited_cells.find(c) == visited_cells.end()){
                cells_to_visit.push(c);
            }
        }
    }
    return false;
}

void Cell::InvalidateCache() const {
    impl_->ResetCache();
    for (Cell* cell_ptr : referring_to_) {
        cell_ptr->InvalidateCache();
    }
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

void Cell::Impl::ResetCache() {
    {
    }
}

Cell::Value Cell::EmptyImpl::GetValue() const {
    return "";
}

std::string Cell::EmptyImpl::GetText() const {
    return "";
}

Cell::TextImpl::TextImpl(std::string text)
        : text_(std::move(text))
{
}

Cell::Value Cell::TextImpl::GetValue() const {
    if (text_.empty()) {
        throw std::logic_error("");
    }
    else if (text_.at(0) == ESCAPE_SIGN) {
        return text_.substr(1);
    }
    else {
        return text_;
    }
}

std::string Cell::TextImpl::GetText() const {
    return text_;
}

Cell::FormulaImpl::FormulaImpl(std::string text, const SheetInterface& sheet_interface)
        : formula_ptr_(ParseFormula(text.substr(1)))
        , sheet_interface_(sheet_interface)
{
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (cache_ == std::nullopt) {
        cache_ = formula_ptr_->Evaluate(sheet_interface_);
    }
    auto formula_ptr_evaluate = formula_ptr_->Evaluate(sheet_interface_);
    if (std::holds_alternative<double>(formula_ptr_evaluate)) {
        return std::get<double>(formula_ptr_evaluate);
    }
    else {
        return std::get<FormulaError>(formula_ptr_evaluate);
    }
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_ptr_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_ptr_->GetReferencedCells();
}

void Cell::FormulaImpl::ResetCache() {
    cache_.reset();
}