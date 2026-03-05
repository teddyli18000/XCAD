#pragma once

#include <afxwin.h>
#include <memory>
#include <vector>

class CShapeManager;
class CLine;

// 命令接口 / command interface
class ICadCommand {
public:
    virtual ~ICadCommand() = default;
    // 功能：执行命令。
    virtual void Execute() = 0;
    // 功能：撤销命令。
    virtual void Undo() = 0;
};

// 添加线条命令 / add line command
class CAddLineCommand : public ICadCommand {
private:
    CShapeManager* m_pManager;
    std::shared_ptr<CLine> m_pLine;

public:
    CAddLineCommand(CShapeManager* mgr, std::shared_ptr<CLine> line);

    void Execute() override;
    void Undo() override;
};

// 平移线条命令 / move lines command
class CMoveLinesCommand : public ICadCommand {
private:
    CShapeManager* m_pManager;
    std::vector<std::shared_ptr<CLine>> m_lines;
    double m_dx;
    double m_dy;
    bool m_hasExecuted;

public:
    CMoveLinesCommand(CShapeManager* mgr, std::vector<std::shared_ptr<CLine>> lines, double dx, double dy, bool alreadyApplied = false);

    void Execute() override;
    void Undo() override;
};

// 修改填充命令 / change fill command
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

// 修改颜色命令 / change color command
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

// 删除线条命令 / delete lines command
class CDeleteLinesCommand : public ICadCommand {
private:
    CShapeManager* m_pManager;
    std::vector<std::shared_ptr<CLine>> m_lines;

public:
    CDeleteLinesCommand(CShapeManager* mgr, std::vector<std::shared_ptr<CLine>> lines);

    void Execute() override;
    void Undo() override;
};

// 替换线条命令 / replace line command
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
