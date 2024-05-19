export module Engine.Core.Exceptions.OperationiCancelledError;
import std;

namespace Engine::Core::Exceptions
{
    export class OperationCancelledError : public std::exception {};
}
