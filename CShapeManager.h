#pragma once

#include "CLine.h"
#include "CViewTransform.h"
#include "Command.h"
#include <memory>
#include <stack>
#include <string>
#include <vector>

class CShapeManager {
private:
    std::vector<std::shared_ptr<CLine>> m_shapes;
    std::stack<std::unique_ptr<ICadCommand>> m_undoStack;
    std::stack<std::unique_ptr<ICadCommand>> m_redoStack;

public:
    void AddShape(std::shared_ptr<CLine> shape);
    void RemoveShape(std::shared_ptr<CLine> shape);
    void Clear();

    void DrawAll(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const;

    void ExecuteCommand(std::unique_ptr<ICadCommand> cmd);
    void Undo();
    void Redo();

    bool SaveToDXF(const std::wstring& filepath) const;
    bool LoadFromDXF(const std::wstring& filepath);
};
