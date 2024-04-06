What To Study:
1. Review what is an ISA relation in ERs
2. Relational Alegbra
   1. Redo the homework assignment centered around this
3. Briefly review all the joins
   1. Make sure I understand all of them and write down the key differences
4. Review aggregates(group by operator)
   1. Just reviewing the notes and marking key points is probably fine for this unit
5. Authorization (10.1)
   1. Just reviewing the notes and marking key points is probably fine for this unit
6. Views and Indexes
   1. Just review how to form a view and materialized vs unmaterialized views
7. Everything about transactions
   1. What is ACID?
   2. Isolation level differences and best for use cases
8. SQL Injection
   1. Just review an example of this, pretty easy unit
9. JDBC Programming
   1. Reivew the syntax, this unit is also pretty simple
________________
Relational Alegbra
    * Select- returns certain rows( need a condition to return on otherwise its the entire table)
    * Weird sideways backwards P
    * Project- Returns certain columns(needs specific column name to return otherwise its full table)
    * Pi symbol
    * Cartesian Product- Returns all rows and data from both tables duplicate column names get marked (table.attribute)
    * Looks like an X
    * Natural Join- Joins two tables enforcing equality on columns with the same name
    * Looks like ⋈
    * Theta Join- Same as natural join but you can chose the condition to force equality on
    * Natural Join with a theta on the side
    * Union- Just combines two different columns into one output
    * U symbol
    * Difference- Same as in discrete, just removes same entires between two specified columns
    * - Symbol
    * Intersection Operator- Same as discrete, shows only the same entries between two columns
    * Intersection symbol
    * Doesn’t add expressive power**

Join Stuff
    * Outer Vs Inner Join
    * Outer join sustains all attributes and uses null values
    * Inner join will cut off attributes and use null values

Authorization
    * grant privledgelist on relation/viewname to userlist
    * privilege list includes stuff like select, update, insert delete, all privileges
    * relation/viewname just means the table or a view you made
    * revoke privilege list on relation/viewname from userlist

Views
    * Materialized Vs Virtual
    * Virtual view is essentially a premade query condensed into a view call
    * Materialized means its actually builds the data table from the view and stores it
Indexes
    * An index is a datastructure used to speed up operations on a table.
    * Syntax is:
        * create index indexName on tableName(attribute)
Transactions:
    * What is a transaction?
        * A transaction is a single unit of logical work, meaning that multiple opearations are done in one sweep
    * What is the purpose of a transaction?
        * Used to ensure that different interactions with tables don't overlap/partially finish
Isolation Levels:
    * Read Uncommited Transactions
        * Can read all data in databases even if it was written by an incomplete transaction
    * Read-Commmited transactions
        * Can only read data that has been committed, can see only complete transaction
    * Repeatable-Read Transactions
        * Like read-committed by if the data gets read again it will reuse the originally read data
    * Serializable
        * Only ACID of the isolation Levels
        * Non Dirty, Non nonrepeatable read
Everything about ACID    
    * What is ACID?(Letter by Letter)
        * Atomic: Treats a transaction as one unit, meaning it either gets fully completely or not ran at all
        * Consistent: Ensures a transaction brings a system to and from a consistent state
            * Preserving data integrity and adhereing to all constraints
        * Isolated: Guarantees multiple transactions can be ran concurrently without affecting one another
        * Durable: Ensures that all commited transactions are durable, i.e they can survive system failures

SQL Injection
    * What is SQL Injection?
        * When a user input that is used in a query can be injected with SQL Syntax to allow them to 'inject' destructive code
        * Can be doing using comments, quotes, etc
    * How to prevent SQL Injection?
        * Escape the special characters from user input, i.e not allow SQL Syntax characters in input
        * Use prepared statements(parameterized queries/transactions)
JDBC Stuff
-- All of this is literally syntax cause the concepts are really easy --
    * Establishing a connection
        * public static Connection getConnection(String url, String user, String password) throws SQLException
    * Create a statement
        * Two options: Prepared or Regular statement
            * Prepared statement is a premade statement that you pass certain values into- secure from SQL Injection
            * Regular statement is just a full query you execute

    * Creating a regular statement looks like this:
        Statement stat1 = conn.createStatement();
        ResultSet results = stat1.executeQuery("Select stuff FROM table" + "WHERE stuff=condition");
        
    * Prepared statement looks like this(painful):
        PreparedStatement stat2 = conn.preparedStatement("SELECT stuff FROM Table " + "WHERE stuff=?");
        
        Set Value of ? like this:
        stat2.setString("condition");
        
        Then execute like this:
        ResultSet results2 = stat2.executeQuery();

    * Accessing results is used with .next(); keyword like this:
        while(results2.next()){
            theResultVal = results.getString('1');
        }

        Brief explanation, next is an iterator/cursour through the result set, it returns false once there's nothing left
        the .getString function needs to be specific to datatype so if its a price it'd be .getFloat
        The value you pass into the .get is the column name
    
    * Transactions with JDBC is really simple
        Start by turning off autoCommit(same as doing start transaction in mysql workbench)
        conn.setAutoCommit(false);
        
        Then set isolation level:
        conn.setTransactionIsolation(conn.TRANSACTION_SERIALIZABLE);

        Do your stuff then commit or rollback if the transaction won't work
        conn.commit();
        or
        conn.rollback();
