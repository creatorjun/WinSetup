// src/abstractions/ui/IPropertyChanged.h
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace winsetup::abstractions {

    using PropertyChangedCallback = std::function<void(const std::wstring& propertyName)>;

    class IPropertyChanged {
    public:
        virtual ~IPropertyChanged() = default;

        virtual void AddPropertyChangedHandler(PropertyChangedCallback callback) = 0;
        virtual void RemoveAllPropertyChangedHandlers() = 0;

    protected:
        virtual void NotifyPropertyChanged(const std::wstring& propertyName) = 0;
    };

}
