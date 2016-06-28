// RUN: backport unique_ptr.cpp -no-db -final-syntax-check

static int alive = 0;
static int shout = 0;
static int destroyed = 0;
static int constructed = 0;

class Person
{
    int age;
    char* pName;

public:
    Person() : pName(0), age(0)
    {
        ++constructed;
    }
    Person(char* pName, int age) : pName(pName), age(age)
    {
        ++constructed;
    }
    ~Person()
    {
        ++destroyed;
    }

    void Display()
    {
        ++alive;
    }
    void Shout()
    {
        ++shout;
    }
};

template < typename T > class UP
{
private:
    T*    pData;       // pointer

public:
    UP() : pData(0)
    {
    }

    UP(T* pValue) : pData(pValue)
    {
    }

    UP(UP<T> &&sp) {
        // not thread safe
        // locking should be used.
        this->pData = sp.pData;
        sp.pData = 0;
    }

    ~UP()
    {
		// not thread safe
        // locking should be used.
        if (pData)
            delete pData;
    }

    T& operator* ()
    {
        return *pData;
    }

    T* operator-> ()
    {
        return pData;
    }

    UP<T>& operator = (UP<T> &&sp)  {
        // not thread safe
        // locking should be used.
        this->pData = sp.pData;
        sp.pData = 0;
		return *this;
    }
	
private:
	UP(const UP<T>& sp){}
    UP<T>& operator = (const UP<T>& sp){}
};

int main()
{

	if (constructed != 0)
		return -1;


	{
		UP<Person> p(new Person("Scott", 25));

		if (constructed != 1)
			return -1;

		if (alive != 0)
			return -1;

		p->Display();
		if (alive != 1) {
			return -1;
		}

		{
			UP<Person> q = (UP<Person> &&)p;
			q->Display();
			if (alive != 2) {
				return -1;
			}

			if (destroyed != 0)
				return -1;
		}

		if (destroyed != 1)
			return -1;
	}

	if (destroyed != 1)
		return -1;

	return 0;
}


