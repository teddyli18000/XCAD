#pragma once

#include <afxwin.h>
#include <memory>
#include <vector>

class CShapeManager;
class CLine;

class ICadCommand {
public:
    virtual ~ICadCommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

class CAddLineCommand : public ICadCommand {
private:
    CShapeManager* m_pManager;
    std::shared_ptr<CLine> m_pLine;

public:
    CAddLineCommand(CShapeManager* mgr, std::shared_ptr<CLine> line);

    void Execute() override;
    void Undo() override;
};

class CChangeLineFillCommand : public ICadCommand {
private:
    CShapeManager* m_pManager;
    std::shared_ptr<CLine> m_line;
    bool m_oldHasFill;
    COLORREF m_oldFillColor;
    bool m_newHasFill;
    COLORREF m_newFillColor;

public:
    CChangeLineFillCommand(CShapeManager* mgr, std::shared_ptr<CLine> line, bool newHasFill, COLORREF newFillColor);

    void Execute() override;
    void Undo() override;
};

class CChangeLineColorCommand : public ICadCommand {
private:
    CShapeManager* m_pManager;
    std::vector<std::shared_ptr<CLine>> m_lines;
    std::vector<COLORREF> m_oldColors;
    COLORREF m_newColor;

public:
    CChangeLineColorCommand(CShapeManager* mgr, std::vector<std::shared_ptr<CLine>> lines, COLORREF newColor);

    void Execute() override;
    void Undo() override;
};

class CDeleteLinesCommand : public ICadCommand {
private:
    CShapeManager* m_pManager;
    std::vector<std::shared_ptr<CLine>> m_lines;

public:
    CDeleteLinesCommand(CShapeManager* mgr, std::vector<std::shared_ptr<CLine>> lines);

    void Execute() override;
    void Undo() override;
};

class CReplaceLineCommand : public ICadCommand {
private:
    CShapeManager* m_pManager;
    std::shared_ptr<CLine> m_original;
    std::vector<std::shared_ptr<CLine>> m_replacements;

public:
    CReplaceLineCommand(CShapeManager* mgr, std::shared_ptr<CLine> original, std::vector<std::shared_ptr<CLine>> replacements);

    void Execute() override;
    void Undo() override;
};
