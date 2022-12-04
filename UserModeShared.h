#pragma once

#define COMMUNICATION_PORT L"\\PathHiderPort"

enum PHAction
{
    AddPathToHideAction,
    RemoveAllhiddenPaths
};

struct PHData
{
    PWCHAR m_path;
    PWCHAR m_name;
};

struct PHMessage
{
    PHAction m_action;
    PHData* m_data;
};
