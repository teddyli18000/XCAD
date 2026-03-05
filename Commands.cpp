#include "pch.h"
#include "Command.h"
#include "CShapeManager.h"
#include <utility>

namespace {
const COLORREF kDefaultCadColor = RGB(255, 255, 255);
}

// 功能：构造“添加线条”命令对象。
CAddLineCommand::CAddLineCommand(CShapeManager* mgr, std::shared_ptr<CLine> line)
    : m_pManager(mgr), m_pLine(std::move(line)) {
}

// 功能：执行添加线条命令。
void CAddLineCommand::Execute() { m_pManager->AddShape(m_pLine); }

// 功能：撤销添加线条命令。
void CAddLineCommand::Undo() { m_pManager->RemoveShape(m_pLine); }

// 功能：构造“删除多条线”命令对象。
CDeleteLinesCommand::CDeleteLinesCommand(CShapeManager* mgr, std::vector<std::shared_ptr<CLine>> lines)
    : m_pManager(mgr), m_lines(std::move(lines)) {
}

// 功能：执行删除线条命令。
void CDeleteLinesCommand::Execute() {
    for (const auto& line : m_lines) {
        m_pManager->RemoveShape(line);
    }
}

// 功能：撤销删除线条命令。
void CDeleteLinesCommand::Undo() {
    for (const auto& line : m_lines) {
        m_pManager->AddShape(line);
    }
}

// 功能：构造“替换线条”命令对象。
CReplaceLineCommand::CReplaceLineCommand(CShapeManager* mgr, std::shared_ptr<CLine> original, std::vector<std::shared_ptr<CLine>> replacements)
    : m_pManager(mgr), m_original(std::move(original)), m_replacements(std::move(replacements)) {
}

// 功能：执行替换线条命令。
void CReplaceLineCommand::Execute() {
    m_pManager->RemoveShape(m_original);
    for (const auto& line : m_replacements) {
        m_pManager->AddShape(line);
    }
}

// 功能：撤销替换线条命令。
void CReplaceLineCommand::Undo() {
    for (const auto& line : m_replacements) {
        m_pManager->RemoveShape(line);
    }
    m_pManager->AddShape(m_original);
}

// 功能：构造“修改线条颜色”命令对象。
CChangeLineColorCommand::CChangeLineColorCommand(CShapeManager* mgr, std::vector<std::shared_ptr<CLine>> lines, COLORREF newColor)
    : m_pManager(mgr), m_lines(std::move(lines)), m_newColor(newColor) {
    m_oldColors.reserve(m_lines.size());
    for (const auto& line : m_lines) {
        m_oldColors.push_back(line ? line->GetColor() : kDefaultCadColor);
    }
}

// 功能：执行颜色修改命令。
void CChangeLineColorCommand::Execute() {
    UNREFERENCED_PARAMETER(m_pManager);
    for (const auto& line : m_lines) {
        if (line) {
            line->SetColor(m_newColor);
        }
    }
}

// 功能：撤销颜色修改命令。
void CChangeLineColorCommand::Undo() {
    for (size_t i = 0; i < m_lines.size() && i < m_oldColors.size(); ++i) {
        if (m_lines[i]) {
            m_lines[i]->SetColor(m_oldColors[i]);
        }
    }
}

// 功能：构造“修改填充状态/颜色”命令对象。
CChangeLineFillCommand::CChangeLineFillCommand(CShapeManager* mgr, std::shared_ptr<CLine> line, bool newHasFill, COLORREF newFillColor)
    : m_pManager(mgr),
    m_line(std::move(line)),
    m_oldHasFill(false),
    m_oldFillColor(kDefaultCadColor),
    m_newHasFill(newHasFill),
    m_newFillColor(newFillColor) {
    if (m_line) {
        m_oldHasFill = m_line->HasFill();
        m_oldFillColor = m_line->GetFillColor();
    }
}

// 功能：执行填充修改命令。
void CChangeLineFillCommand::Execute() {
    UNREFERENCED_PARAMETER(m_pManager);
    if (m_line) {
        m_line->SetFill(m_newHasFill, m_newFillColor);
    }
}

// 功能：撤销填充修改命令。
void CChangeLineFillCommand::Undo() {
    if (m_line) {
        m_line->SetFill(m_oldHasFill, m_oldFillColor);
    }
}

// 功能：构造“平移多条线”命令对象。
CMoveLinesCommand::CMoveLinesCommand(CShapeManager* mgr, std::vector<std::shared_ptr<CLine>> lines, double dx, double dy, bool alreadyApplied)
    : m_pManager(mgr),
    m_lines(std::move(lines)),
    m_dx(dx),
    m_dy(dy),
    m_hasExecuted(alreadyApplied) {
}

// 功能：执行平移命令。
void CMoveLinesCommand::Execute() {
    UNREFERENCED_PARAMETER(m_pManager);
    if (!m_hasExecuted) {
        for (const auto& line : m_lines) {
            if (line) {
                line->Move(m_dx, m_dy);
            }
        }
    }
    m_hasExecuted = true;
}

// 功能：撤销平移命令。
void CMoveLinesCommand::Undo() {
    if (!m_hasExecuted) return;
    for (const auto& line : m_lines) {
        if (line) {
            line->Move(-m_dx, -m_dy);
        }
    }
    m_hasExecuted = false;
}
