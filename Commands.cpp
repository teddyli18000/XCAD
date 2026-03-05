#include "pch.h"
#include "Command.h"
#include "CShapeManager.h"
#include <utility>

CAddLineCommand::CAddLineCommand(CShapeManager* mgr, std::shared_ptr<CLine> line)
    : m_pManager(mgr), m_pLine(std::move(line)) {
}

void CAddLineCommand::Execute() { m_pManager->AddShape(m_pLine); }

void CAddLineCommand::Undo() { m_pManager->RemoveShape(m_pLine); }

CDeleteLinesCommand::CDeleteLinesCommand(CShapeManager* mgr, std::vector<std::shared_ptr<CLine>> lines)
    : m_pManager(mgr), m_lines(std::move(lines)) {
}

void CDeleteLinesCommand::Execute() {
    for (const auto& line : m_lines) {
        m_pManager->RemoveShape(line);
    }
}

void CDeleteLinesCommand::Undo() {
    for (const auto& line : m_lines) {
        m_pManager->AddShape(line);
    }
}

CReplaceLineCommand::CReplaceLineCommand(CShapeManager* mgr, std::shared_ptr<CLine> original, std::vector<std::shared_ptr<CLine>> replacements)
    : m_pManager(mgr), m_original(std::move(original)), m_replacements(std::move(replacements)) {
}

void CReplaceLineCommand::Execute() {
    m_pManager->RemoveShape(m_original);
    for (const auto& line : m_replacements) {
        m_pManager->AddShape(line);
    }
}

void CReplaceLineCommand::Undo() {
    for (const auto& line : m_replacements) {
        m_pManager->RemoveShape(line);
    }
    m_pManager->AddShape(m_original);
}

CChangeLineColorCommand::CChangeLineColorCommand(CShapeManager* mgr, std::vector<std::shared_ptr<CLine>> lines, COLORREF newColor)
    : m_pManager(mgr), m_lines(std::move(lines)), m_newColor(newColor) {
    m_oldColors.reserve(m_lines.size());
    for (const auto& line : m_lines) {
        m_oldColors.push_back(line ? line->GetColor() : RGB(255, 255, 255));
    }
}

void CChangeLineColorCommand::Execute() {
    UNREFERENCED_PARAMETER(m_pManager);
    for (const auto& line : m_lines) {
        if (line) {
            line->SetColor(m_newColor);
        }
    }
}

void CChangeLineColorCommand::Undo() {
    for (size_t i = 0; i < m_lines.size() && i < m_oldColors.size(); ++i) {
        if (m_lines[i]) {
            m_lines[i]->SetColor(m_oldColors[i]);
        }
    }
}

CChangeLineFillCommand::CChangeLineFillCommand(CShapeManager* mgr, std::shared_ptr<CLine> line, bool newHasFill, COLORREF newFillColor)
    : m_pManager(mgr),
    m_line(std::move(line)),
    m_oldHasFill(false),
    m_oldFillColor(RGB(255, 255, 255)),
    m_newHasFill(newHasFill),
    m_newFillColor(newFillColor) {
    if (m_line) {
        m_oldHasFill = m_line->HasFill();
        m_oldFillColor = m_line->GetFillColor();
    }
}

void CChangeLineFillCommand::Execute() {
    UNREFERENCED_PARAMETER(m_pManager);
    if (m_line) {
        m_line->SetFill(m_newHasFill, m_newFillColor);
    }
}

void CChangeLineFillCommand::Undo() {
    if (m_line) {
        m_line->SetFill(m_oldHasFill, m_oldFillColor);
    }
}
