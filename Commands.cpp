#include "pch.h"
#include "Command.h"
#include "CShapeManager.h"

CAddLineCommand::CAddLineCommand(CShapeManager* mgr, std::shared_ptr<CLine> line)
    : m_pManager(mgr), m_pLine(std::move(line)) {
}

void CAddLineCommand::Execute() { m_pManager->AddShape(m_pLine); }

void CAddLineCommand::Undo() { m_pManager->RemoveShape(m_pLine); }
