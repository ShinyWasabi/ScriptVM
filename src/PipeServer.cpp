#include "PipeServer.hpp"
#include "PipeCommands.hpp"
#include "ScriptBreakpoint.hpp"

PipeServer::PipeServer() :
    m_PipeHandle(INVALID_HANDLE_VALUE)
{
}

PipeServer::~PipeServer()
{
    DestroyImpl();
}

bool PipeServer::InitImpl(const std::string& name)
{
    if (m_PipeHandle != INVALID_HANDLE_VALUE)
        return false;

    m_PipeHandle = CreateNamedPipeA(("\\\\.\\pipe\\" + name).c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 4096, 4096, 0, nullptr);

    return m_PipeHandle != INVALID_HANDLE_VALUE;
}

void PipeServer::DestroyImpl()
{
    if (m_PipeHandle != INVALID_HANDLE_VALUE)
    {
        DisconnectNamedPipe(m_PipeHandle);
        CloseHandle(m_PipeHandle);
        m_PipeHandle = INVALID_HANDLE_VALUE;
    }
}

void PipeServer::RunImpl()
{
    while (true)
    {
        if (!Wait())
            continue;

        bool disconnected = false;
        while (!disconnected)
        {
            std::uint8_t cmdByte = 0;
            if (!Receive(&cmdByte, sizeof(cmdByte)))
            {
                disconnected = true;
                break;
            }

            auto cmd = static_cast<ePipeCommands>(cmdByte);
            switch (cmd)
            {
            case ePipeCommands::BREAKPOINT_SET:
            {
                PipeCommandSetBreakpoint args{};
                if (!Receive(&args, sizeof(args)))
                {
                    disconnected = true;
                    break;
                }

                if (args.Set)
                    ScriptBreakpoint::Add(args.Script, args.Pc);
                else
                    ScriptBreakpoint::Remove(args.Script, args.Pc);
                break;
            }
            case ePipeCommands::BREAKPOINT_EXISTS:
            {
                PipeCommandBreakpointExists args{};
                if (!Receive(&args, sizeof(args)))
                {
                    disconnected = true;
                    break;
                }

                uint8_t result = ScriptBreakpoint::Exists(args.Script, args.Pc) ? 1 : 0;
                Send(&result, sizeof(result));
                break;
            }
            case ePipeCommands::BREAKPOINT_ACTIVE:
            {
                uint8_t result = ScriptBreakpoint::Active() ? 1 : 0;
                Send(&result, sizeof(result));
                break;
            }
            case ePipeCommands::BREAKPOINT_RESUME:
            {
                ScriptBreakpoint::Resume();
                break;
            }
            case ePipeCommands::BREAKPOINT_GET_ALL:
            {
                const auto bps = ScriptBreakpoint::GetAll();

                uint32_t count = static_cast<uint32_t>(bps.size());
                Send(&count, sizeof(count));

                for (const auto& bp : bps)
                {
                    PipeCommandBreakpointGetAll entry{ bp.Script, bp.Pc };
                    Send(&entry, sizeof(entry));
                }

                break;
            }
            case ePipeCommands::BREAKPOINT_REMOVE_ALL:
            {
                ScriptBreakpoint::RemoveAll();
                break;
            }
            default:
                // Do nothing?
                break;
            }
        }

        DisconnectNamedPipe(m_PipeHandle);
    }
}

bool PipeServer::Wait()
{
    if (m_PipeHandle == INVALID_HANDLE_VALUE)
        return false;

    if (ConnectNamedPipe(m_PipeHandle, nullptr))
        return true;

    return GetLastError() == ERROR_PIPE_CONNECTED;
}

bool PipeServer::Send(const void* data, size_t size)
{
    if (m_PipeHandle == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;
    return WriteFile(m_PipeHandle, data, static_cast<DWORD>(size), &written, nullptr) && written == size;
}

bool PipeServer::Receive(void* data, size_t size)
{
    if (m_PipeHandle == INVALID_HANDLE_VALUE)
        return false;

    DWORD read = 0;
    return ReadFile(m_PipeHandle, data, static_cast<DWORD>(size), &read, nullptr) && read == size;
}