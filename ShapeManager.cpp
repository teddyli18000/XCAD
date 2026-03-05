#include "pch.h"
#include "CShapeManager.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <utility>

namespace {
std::string Trim(const char* text) {
    std::string s = text ? text : "";
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || std::isspace(static_cast<unsigned char>(s.back())))) {
        s.pop_back();
    }
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    return s.substr(start);
}

int ColorToDxfAci(COLORREF color) {
    if (color == RGB(255, 0, 0)) return 1;
    if (color == RGB(255, 255, 0)) return 2;
    if (color == RGB(0, 255, 0)) return 3;
    if (color == RGB(0, 255, 255)) return 4;
    if (color == RGB(0, 0, 255)) return 5;
    if (color == RGB(255, 0, 255)) return 6;
    return 7;
}

COLORREF DxfAciToColor(int aci) {
    switch (aci) {
    case 1: return RGB(255, 0, 0);
    case 2: return RGB(255, 255, 0);
    case 3: return RGB(0, 255, 0);
    case 4: return RGB(0, 255, 255);
    case 5: return RGB(0, 0, 255);
    case 6: return RGB(255, 0, 255);
    case 7:
    default:
        return RGB(255, 255, 255);
    }
}
}

void CShapeManager::AddShape(std::shared_ptr<CLine> shape) {
    m_shapes.push_back(std::move(shape));
}

void CShapeManager::RemoveShape(std::shared_ptr<CLine> shape) {
    auto it = std::find(m_shapes.begin(), m_shapes.end(), shape);
    if (it != m_shapes.end()) m_shapes.erase(it);
}

void CShapeManager::Clear() {
    m_shapes.clear();
    while (!m_undoStack.empty()) m_undoStack.pop();
    while (!m_redoStack.empty()) m_redoStack.pop();
}

std::vector<std::shared_ptr<CLine>>& CShapeManager::GetShapes() {
    return m_shapes;
}

const std::vector<std::shared_ptr<CLine>>& CShapeManager::GetShapes() const {
    return m_shapes;
}

void CShapeManager::DrawAll(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const {
    for (const auto& shape : m_shapes) {
        shape->Draw(pDC, transform, bShowPoints);
    }
}

void CShapeManager::ExecuteCommand(std::unique_ptr<ICadCommand> cmd) {
    cmd->Execute();
    m_undoStack.push(std::move(cmd));
    while (!m_redoStack.empty()) m_redoStack.pop();
}

void CShapeManager::Undo() {
    if (!m_undoStack.empty()) {
        std::unique_ptr<ICadCommand> cmd = std::move(m_undoStack.top());
        m_undoStack.pop();
        cmd->Undo();
        m_redoStack.push(std::move(cmd));
    }
}

void CShapeManager::Redo() {
    if (!m_redoStack.empty()) {
        std::unique_ptr<ICadCommand> cmd = std::move(m_redoStack.top());
        m_redoStack.pop();
        cmd->Execute();
        m_undoStack.push(std::move(cmd));
    }
}

bool CShapeManager::SaveToDXF(const std::wstring& filepath) const {
    FILE* fp = nullptr;
    _wfopen_s(&fp, filepath.c_str(), L"w");
    if (!fp) return false;

    fprintf(fp, "  0\nSECTION\n  2\nENTITIES\n");
    for (const auto& shape : m_shapes) {
        const auto& pts = shape->GetPoints();
        if (pts.size() < 2) continue;

        fprintf(fp, "  0\nPOLYLINE\n  8\n0\n 62\n%d\n 66\n1\n", ColorToDxfAci(shape->GetColor()));
        for (const auto& pt : pts) {
            fprintf(fp, "  0\nVERTEX\n  8\n0\n 10\n%f\n 20\n%f\n 30\n0.0\n", pt.x, pt.y);
        }
        fprintf(fp, "  0\nSEQEND\n");
    }
    fprintf(fp, "  0\nENDSEC\n  0\nEOF\n");
    fclose(fp);
    return true;
}

bool CShapeManager::LoadFromDXF(const std::wstring& filepath) {
    FILE* fp = nullptr;
    _wfopen_s(&fp, filepath.c_str(), L"r");
    if (!fp) return false;

    Clear();

    char codeBuffer[128] = {};
    char valueBuffer[256] = {};
    std::shared_ptr<CLine> currentLine;
    bool hasPendingX = false;
    double pendingX = 0.0;

    while (fgets(codeBuffer, sizeof(codeBuffer), fp) != nullptr) {
        if (fgets(valueBuffer, sizeof(valueBuffer), fp) == nullptr) break;

        int code = std::atoi(Trim(codeBuffer).c_str());
        std::string value = Trim(valueBuffer);

        if (code == 0 && value == "POLYLINE") {
            currentLine = std::make_shared<CLine>();
            currentLine->SetColor(RGB(255, 255, 255));
            hasPendingX = false;
            continue;
        }

        if (code == 0 && value == "SEQEND") {
            if (currentLine && currentLine->GetPoints().size() >= 2) {
                m_shapes.push_back(currentLine);
            }
            currentLine.reset();
            hasPendingX = false;
            continue;
        }

        if (!currentLine) continue;

        if (code == 10) {
            pendingX = std::atof(value.c_str());
            hasPendingX = true;
        } else if (code == 20 && hasPendingX) {
            double y = std::atof(value.c_str());
            currentLine->AddPoint(Point2D(pendingX, y));
            hasPendingX = false;
        } else if (code == 62) {
            currentLine->SetColor(DxfAciToColor(std::atoi(value.c_str())));
        }
    }

    fclose(fp);
    return true;
}
