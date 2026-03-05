#include "pch.h"
#include "CShapeManager.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <string>
#include <utility>

namespace {
const COLORREF kCadColorWhite = RGB(255, 255, 255);
const COLORREF kCadColorRed = RGB(255, 0, 0);
const COLORREF kCadColorYellow = RGB(255, 255, 0);
const COLORREF kCadColorGreen = RGB(0, 255, 0);
const COLORREF kCadColorCyan = RGB(0, 255, 255);
const COLORREF kCadColorBlue = RGB(0, 0, 255);
const COLORREF kCadColorMagenta = RGB(255, 0, 255);

const int kDxfAciRed = 1;
const int kDxfAciYellow = 2;
const int kDxfAciGreen = 3;
const int kDxfAciCyan = 4;
const int kDxfAciBlue = 5;
const int kDxfAciMagenta = 6;
const int kDxfAciWhite = 7;

const int kDxfMinPolylinePoints = 2;
const char* kDxfMetaAppName = "CAD_ENTITY_META";
const char* kDxfMetaKeyEntityType = "ET";
const char* kDxfMetaKeyCenterX = "CX";
const char* kDxfMetaKeyCenterY = "CY";
const char* kDxfMetaKeyRadius = "R";
const char* kDxfMetaKeyStartAngle = "SA";
const char* kDxfMetaKeyEndAngle = "EA";

//去除文本两端空白，便于解析 DXF 行内容
std::string Trim(const std::string& text) {
    std::string s = text;
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || std::isspace(static_cast<unsigned char>(s.back())))) {
        s.pop_back();
    }
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    return s.substr(start);
}

//把 RGB 颜色映射为 DXF ACI 颜色索引
int ColorToDxfAci(COLORREF color) {
    if (color == kCadColorRed) return kDxfAciRed;
    if (color == kCadColorYellow) return kDxfAciYellow;
    if (color == kCadColorGreen) return kDxfAciGreen;
    if (color == kCadColorCyan) return kDxfAciCyan;
    if (color == kCadColorBlue) return kDxfAciBlue;
    if (color == kCadColorMagenta) return kDxfAciMagenta;
    return kDxfAciWhite;
}

//把 DXF ACI 颜色索引映射回 RGB
COLORREF DxfAciToColor(int aci) {
    switch (aci) {
    case kDxfAciRed: return kCadColorRed;
    case kDxfAciYellow: return kCadColorYellow;
    case kDxfAciGreen: return kCadColorGreen;
    case kDxfAciCyan: return kCadColorCyan;
    case kDxfAciBlue: return kCadColorBlue;
    case kDxfAciMagenta: return kCadColorMagenta;
    case kDxfAciWhite:
    default:
        return kCadColorWhite;
    }
}

//text to int, for DXF metadata storage
int EntityTypeToInt(EntityType type) {
    switch (type) {
    case EntityType::LINE: return 0;
    case EntityType::CIRCLE: return 1;
    case EntityType::ARC: return 2;
    case EntityType::RECTANGLE: return 3;
    default: return 0;
    }
}

//int to text, for DXF metadata parsing
EntityType IntToEntityType(int value) {
    switch (value) {
    case 1: return EntityType::CIRCLE;
    case 2: return EntityType::ARC;
    case 3: return EntityType::RECTANGLE;
    case 0:
    default:
        return EntityType::LINE;
    }
}

void AppendEntityMetadata(std::ostringstream& dxf, const CLine& shape) {
    const EntityData& data = shape.GetEntityData();
    dxf << "1001\n" << kDxfMetaAppName
        << "\n1000\n" << kDxfMetaKeyEntityType
        << "\n1070\n" << EntityTypeToInt(shape.GetEntityType())
        << "\n1000\n" << kDxfMetaKeyCenterX
        << "\n1040\n" << data.Center.x
        << "\n1000\n" << kDxfMetaKeyCenterY
        << "\n1040\n" << data.Center.y
        << "\n1000\n" << kDxfMetaKeyRadius
        << "\n1040\n" << data.Radius
        << "\n1000\n" << kDxfMetaKeyStartAngle
        << "\n1040\n" << data.StartAngle
        << "\n1000\n" << kDxfMetaKeyEndAngle
        << "\n1040\n" << data.EndAngle
        << "\n";
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int size = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string utf8(size, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &utf8[0], size, nullptr, nullptr);
    return utf8;
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) return {};
    const int size = ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) return {};
    std::wstring wide(size, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &wide[0], size);
    return wide;
}
}

//向图元列表添加一个图形对象
void CShapeManager::AddShape(std::shared_ptr<CLine> shape) {
    m_shapes.push_back(std::move(shape));
}

//从图元列表移除指定图形对象
void CShapeManager::RemoveShape(std::shared_ptr<CLine> shape) {
    auto it = std::find(m_shapes.begin(), m_shapes.end(), shape);
    if (it != m_shapes.end()) m_shapes.erase(it);
}

//清空图形与撤销重做栈
void CShapeManager::Clear() {
    m_shapes.clear();
    while (!m_undoStack.empty()) m_undoStack.pop();
    while (!m_redoStack.empty()) m_redoStack.pop();
    m_historyIndex = 0;
    m_savedHistoryIndex = 0;
}

//返回可写的图形容器引用
std::vector<std::shared_ptr<CLine>>& CShapeManager::GetShapes() {
    return m_shapes;
}

//返回只读的图形容器引用
const std::vector<std::shared_ptr<CLine>>& CShapeManager::GetShapes() const {
    return m_shapes;
}

//绘制所有图元
void CShapeManager::DrawAll(CDC* pDC, const CViewTransform& transform, bool bShowPoints) const {
    for (const auto& shape : m_shapes) {
        shape->Draw(pDC, transform, bShowPoints);
    }
}

//执行命令并压入撤销栈
void CShapeManager::ExecuteCommand(std::unique_ptr<ICadCommand> cmd) {
    cmd->Execute();
    m_undoStack.push(std::move(cmd));
    while (!m_redoStack.empty()) m_redoStack.pop();
    ++m_historyIndex;
}

//执行撤销操作
void CShapeManager::Undo() {
    if (!m_undoStack.empty()) {
        std::unique_ptr<ICadCommand> cmd = std::move(m_undoStack.top());
        m_undoStack.pop();
        cmd->Undo();
        m_redoStack.push(std::move(cmd));
        --m_historyIndex;
    }
}

//执行重做操作
void CShapeManager::Redo() {
    if (!m_redoStack.empty()) {
        std::unique_ptr<ICadCommand> cmd = std::move(m_redoStack.top());
        m_redoStack.pop();
        cmd->Execute();
        m_undoStack.push(std::move(cmd));
        ++m_historyIndex;
    }
}

//将当前编辑状态标记为已保存
void CShapeManager::MarkSaved() {
    m_savedHistoryIndex = m_historyIndex;
}

//判断当前是否有未保存修改
bool CShapeManager::HasUnsavedChanges() const {
    return m_historyIndex != m_savedHistoryIndex;
}

//将当前图元导出为 DXF 文件
bool CShapeManager::SaveToDXF(const std::wstring& filepath) const {
    std::ostringstream dxf;
    dxf << "  0\nSECTION\n  2\nENTITIES\n";

    for (const auto& shape : m_shapes) {
        if (!shape) continue;
        const auto& pts = shape->GetPoints();

        if (shape->IsTextEntity()) {
            if (pts.size() < 3) continue;
            const Point2D& p1 = pts[0];
            const Point2D& p3 = pts[2];
            const double textHeight = (std::max)(std::fabs(p3.y - p1.y) * 0.8, 1.0);
            dxf << "  0\nTEXT\n  8\n0\n 62\n" << ColorToDxfAci(shape->GetColor())
                << "\n 10\n" << p1.x
                << "\n 20\n" << p1.y
                << "\n 11\n" << p3.x
                << "\n 21\n" << p3.y
                << "\n 40\n" << textHeight
                << "\n  1\n" << WideToUtf8(shape->GetTextContent())
                << "\n";
            AppendEntityMetadata(dxf, *shape);
            continue;
        }

        if (pts.size() < kDxfMinPolylinePoints) continue;

        dxf << "  0\nPOLYLINE\n  8\n0\n 62\n" << ColorToDxfAci(shape->GetColor())
            << "\n 450\n" << (shape->HasFill() ? 1 : 0)
            << "\n 451\n" << ColorToDxfAci(shape->GetFillColor())
            << "\n 66\n1\n";
        for (const auto& pt : pts) {
            dxf << "  0\nVERTEX\n  8\n0\n 10\n" << pt.x << "\n 20\n" << pt.y << "\n 30\n0.0\n";
        }
        AppendEntityMetadata(dxf, *shape);
        dxf << "  0\nSEQEND\n";
    }

    dxf << "  0\nENDSEC\n  0\nEOF\n";

    FILE* outFile = nullptr;
    _wfopen_s(&outFile, filepath.c_str(), L"wb");
    if (!outFile) return false;

    const unsigned char utf8Bom[] = { 0xEF, 0xBB, 0xBF };
    fwrite(utf8Bom, 1, sizeof(utf8Bom), outFile);
    const std::string content = dxf.str();
    fwrite(content.data(), 1, content.size(), outFile);
    fclose(outFile);
    return true;
}

//从 DXF 文件加载图元数据
bool CShapeManager::LoadFromDXF(const std::wstring& filepath) {
    FILE* inFile = nullptr;
    _wfopen_s(&inFile, filepath.c_str(), L"rb");
    if (!inFile) return false;

    std::string fileData;
    char buffer[4096] = {};
    size_t readSize = 0;
    while ((readSize = fread(buffer, 1, sizeof(buffer), inFile)) > 0) {
        fileData.append(buffer, readSize);
    }
    fclose(inFile);
    if (fileData.size() >= 3
        && static_cast<unsigned char>(fileData[0]) == 0xEF
        && static_cast<unsigned char>(fileData[1]) == 0xBB
        && static_cast<unsigned char>(fileData[2]) == 0xBF) {
        fileData.erase(0, 3);
    }

    Clear();

    std::shared_ptr<CLine> currentLine;
    bool hasPendingX = false;
    double pendingX = 0.0;
    bool textHasX1 = false;
    bool textHasY1 = false;
    bool textHasX2 = false;
    bool textHasY2 = false;
    double textX1 = 0.0;
    double textY1 = 0.0;
    double textX2 = 0.0;
    double textY2 = 0.0;
    EntityData pendingEntityData;
    bool hasPendingEntityData = false;
    bool hasPendingEntityType = false;
    EntityType pendingEntityType = EntityType::LINE;
    bool readingMetaData = false;
    std::string lastMetaKey;

    auto finalizeCurrentShape = [&]() {
        if (!currentLine) return;

        if (hasPendingEntityType) {
            currentLine->SetEntityType(pendingEntityType);
        }
        if (hasPendingEntityData) {
            currentLine->SetEntityData(pendingEntityData);
        }

        if (currentLine->IsTextEntity()) {
            if (textHasX1 && textHasY1 && textHasX2 && textHasY2) {
                currentLine->AddPoint(Point2D(textX1, textY1));
                currentLine->AddPoint(Point2D(textX2, textY1));
                currentLine->AddPoint(Point2D(textX2, textY2));
                currentLine->AddPoint(Point2D(textX1, textY2));
                currentLine->AddPoint(Point2D(textX1, textY1));
                m_shapes.push_back(currentLine);
            }
        } else if (currentLine->GetPoints().size() >= kDxfMinPolylinePoints) {
            m_shapes.push_back(currentLine);
        }

        currentLine.reset();
        hasPendingX = false;
        textHasX1 = false;
        textHasY1 = false;
        textHasX2 = false;
        textHasY2 = false;
        pendingEntityData = EntityData();
        hasPendingEntityData = false;
        hasPendingEntityType = false;
        pendingEntityType = EntityType::LINE;
        readingMetaData = false;
        lastMetaKey.clear();
    };

    std::vector<std::string> lines;
    std::istringstream lineStream(fileData);
    for (std::string line; std::getline(lineStream, line);) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }

    for (size_t i = 0; i + 1 < lines.size(); i += 2) {
        int code = std::atoi(Trim(lines[i]).c_str());
        std::string value = Trim(lines[i + 1]);

        if (code == 0 && value == "POLYLINE") {
            finalizeCurrentShape();
            currentLine = std::make_shared<CLine>();
            currentLine->SetColor(kCadColorWhite);
            currentLine->SetFill(false, kCadColorWhite);
            hasPendingX = false;
            pendingEntityData = EntityData();
            hasPendingEntityData = false;
            hasPendingEntityType = false;
            readingMetaData = false;
            lastMetaKey.clear();
            continue;
        }

        if (code == 0 && value == "TEXT") {
            finalizeCurrentShape();
            currentLine = std::make_shared<CLine>();
            currentLine->SetTextEntity(true);
            currentLine->SetColor(kCadColorWhite);
            pendingEntityData = EntityData();
            hasPendingEntityData = false;
            hasPendingEntityType = false;
            readingMetaData = false;
            lastMetaKey.clear();
            continue;
        }

        if (code == 0 && value == "SEQEND") {
            finalizeCurrentShape();
            continue;
        }

        if (code == 0 && value == "VERTEX") {
            continue;
        }

        if (code == 0) {
            finalizeCurrentShape();
            continue;
        }

        if (!currentLine) continue;

        if (code == 1001) {
            readingMetaData = (value == kDxfMetaAppName);
            lastMetaKey.clear();
            continue;
        }

        if (readingMetaData) {
            if (code == 1000) {
                lastMetaKey = value;
                continue;
            }

            if (lastMetaKey == kDxfMetaKeyEntityType && code == 1070) {
                pendingEntityType = IntToEntityType(std::atoi(value.c_str()));
                hasPendingEntityType = true;
                continue;
            }

            if (code == 1040) {
                if (lastMetaKey == kDxfMetaKeyCenterX) {
                    pendingEntityData.Center.x = std::atof(value.c_str());
                    hasPendingEntityData = true;
                    continue;
                }
                if (lastMetaKey == kDxfMetaKeyCenterY) {
                    pendingEntityData.Center.y = std::atof(value.c_str());
                    hasPendingEntityData = true;
                    continue;
                }
                if (lastMetaKey == kDxfMetaKeyRadius) {
                    pendingEntityData.Radius = std::atof(value.c_str());
                    hasPendingEntityData = true;
                    continue;
                }
                if (lastMetaKey == kDxfMetaKeyStartAngle) {
                    pendingEntityData.StartAngle = std::atof(value.c_str());
                    hasPendingEntityData = true;
                    continue;
                }
                if (lastMetaKey == kDxfMetaKeyEndAngle) {
                    pendingEntityData.EndAngle = std::atof(value.c_str());
                    hasPendingEntityData = true;
                    continue;
                }
            }
        }

        if (currentLine->IsTextEntity()) {
            if (code == 10) {
                textX1 = std::atof(value.c_str());
                textHasX1 = true;
            } else if (code == 20) {
                textY1 = std::atof(value.c_str());
                textHasY1 = true;
            } else if (code == 11) {
                textX2 = std::atof(value.c_str());
                textHasX2 = true;
            } else if (code == 21) {
                textY2 = std::atof(value.c_str());
                textHasY2 = true;
            } else if (code == 62) {
                currentLine->SetColor(DxfAciToColor(std::atoi(value.c_str())));
            } else if (code == 1) {
                currentLine->SetTextContent(Utf8ToWide(value));
            }
            continue;
        }

        if (code == 10) {
            pendingX = std::atof(value.c_str());
            hasPendingX = true;
        } else if (code == 20 && hasPendingX) {
            double y = std::atof(value.c_str());
            currentLine->AddPoint(Point2D(pendingX, y));
            hasPendingX = false;
        } else if (code == 62) {
            currentLine->SetColor(DxfAciToColor(std::atoi(value.c_str())));
        } else if (code == 450) {
            const bool hasFill = std::atoi(value.c_str()) != 0;
            currentLine->SetFill(hasFill, currentLine->GetFillColor());
        } else if (code == 451) {
            currentLine->SetFill(currentLine->HasFill(), DxfAciToColor(std::atoi(value.c_str())));
        }
    }

    finalizeCurrentShape();

    return true;
}
