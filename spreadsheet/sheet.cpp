#include "cell.h"
#include "common.h"
#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (pos.IsValid()) {
        const auto& cell = cells_.find(pos);
        if (cell == cells_.end()) {
            cells_.emplace(pos, std::make_unique<Cell>(*this));
        }
        cells_.at(pos)->Set(std::move(text));
    } else {
        throw InvalidPositionException("Invalid Cell position");
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetConcreteCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetConcreteCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (pos.IsValid()) {
        const auto& cell = cells_.find(pos);
        if (cell != cells_.end() && cell->second != nullptr) {
            cell->second->Clear();
            cell->second.reset();
        }
    } else {
        throw InvalidPositionException("Invalid Cell position");
    }
}

Size Sheet::GetPrintableSize() const {
    Size printable_size { 0, 0 };
    for (auto pos = cells_.begin(); pos != cells_.end(); ++pos) {
        if (pos->second != nullptr) {
            const int row = pos->first.row;
            const int col = pos->first.col;
            printable_size.rows = std::max(printable_size.rows, row + 1);
            printable_size.cols = std::max(printable_size.cols, col + 1);
        }
    }
    return printable_size;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size printable_size = GetPrintableSize();
    for (int row = 0; row < printable_size.rows; ++row) {
        for (int col = 0; col < printable_size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            const auto& pos = cells_.find({ row, col });
            if (pos != cells_.end() && pos->second != nullptr && !pos->second->GetText().empty()) {
                std::visit([&](const auto val) {
                    output << val;
                }, pos->second->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size printable_size = GetPrintableSize();
    for (int row = 0; row < printable_size.rows; ++row) {
        for (int col = 0; col < printable_size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            const auto& pos = cells_.find({ row, col });
            if (pos != cells_.end() && pos->second != nullptr && !pos->second->GetText().empty()) {
                output << pos->second->GetText();
            }
        }
        output << '\n';
    }
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid Cell position");
    }
    const auto cell = cells_.find(pos);
    if (cell == cells_.end()) {
        return nullptr;
    }
    else {
        return cells_.at(pos).get();
    }
}

Cell* Sheet::GetConcreteCell(Position pos) {
    return const_cast<Cell*>(static_cast<const Sheet&>(*this).GetConcreteCell(pos));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}