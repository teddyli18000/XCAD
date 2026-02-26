#pragma once

#include <memory>

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
