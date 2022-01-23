//
// Copyright (c) 2020-2022 Theophilus Eriata.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

#pragma once

#include "../Core/Object.h"
#include "../Database/DbConnection.h"

namespace Urho3D
{

/// Supported database API.
enum DBAPI
{
    DBAPI_SQLITE = 0,
    DBAPI_ODBC
};

class DbConnection;

/// %Database subsystem. Manage database connections.
class URHO3D_API Database : public Object
{
    URHO3D_OBJECT(Database, Object);

public:
    /// Construct.
    explicit Database(Context* context);
    /// Return the underlying database API.
    static DBAPI GetAPI();

    /// Create new database connection. Return 0 if failed.
    DbConnection* Connect(const ea::string& connectionString);
    /// Disconnect a database connection. The connection object pointer should not be used anymore after this.
    void Disconnect(DbConnection* connection);

    /// Return true when using internal database connection pool. The internal database pool is managed by the Database
    /// subsystem itself and should not be confused with ODBC connection pool option when ODBC is being used.
    /// @property
    bool IsPooling() const { return (bool)poolSize_; }

    /// Get internal database connection pool size.
    /// @property
    unsigned GetPoolSize() const { return poolSize_; }

    /// Set internal database connection pool size.
    /// @property
    void SetPoolSize(unsigned poolSize) { poolSize_ = poolSize; }

private:
    /// %Database connection pool size. Default to 0 when using ODBC 3.0 or later as ODBC 3.0 driver manager could
    /// manage its own database connection pool.
    unsigned poolSize_;
    /// Active database connections.
    ea::vector<SharedPtr<DbConnection>> connections_;
    ///%Database connections pool.
    ea::hash_map<ea::string, ea::vector<SharedPtr<DbConnection>>> connectionsPool_;
};

} // namespace Urho3D
