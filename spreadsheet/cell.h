#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    void InvalidateCache() const;

private:
    class Impl {
    public:
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const;
        virtual void ResetCache();
        virtual ~Impl() = default;
    };

    class EmptyImpl : public Impl {
    public:
        Value GetValue() const override;
        std::string GetText() const override;
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string text);
        Value GetValue() const override;
        std::string GetText() const override;

    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(std::string text, const SheetInterface& sheet_interface);
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
        void ResetCache() override;

    private:
        std::unique_ptr<FormulaInterface> formula_ptr_;
        const SheetInterface& sheet_interface_;
        mutable std::optional<FormulaInterface::Value> cache_;
    };

    bool IsReferenced(const Impl& impl) const;

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;

    std::unordered_set<Cell*> referring_to_; // cyclic dependencies
    std::unordered_set<Cell*> referring_from_; // cache invalidation
};