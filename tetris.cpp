// Teris.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <string>
#include <conio.h>
#include <Windows.h>
#include <thread>
#include <mutex>
#include <ctime>
#include <random>
#include <queue>

using namespace std;

queue<int> input_queue;

const int MAX_MAP_HEIGHT = 20, MAX_MAP_WEIGHT = 14;

std::string ShowMap[MAX_MAP_HEIGHT] =
{
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"|            |",
	"--------------",
};

enum class KeyboardInput {
	UP = 72, LEFT = 75, RIGHT = 77, DOWN = 80, SPACE = 32
};

class Object
{
private:
	int X_, Y_, GraphHeight_, GraphWeight_;

protected:
	std::string* Graph_;

public:
	Object(int InitX, int InitY, int InitHeight, int InitWeight) :
		X_(InitX),
		Y_(InitY),
		GraphHeight_(InitHeight),
		GraphWeight_(InitWeight)
	{
		Graph_ = new std::string[GraphHeight_];
	}

	int GetX() const { return X_; }
	int GetY() const { return Y_; }

	int GetGraphHeight() const { return GraphHeight_; }
	int GetGraphWeight() const { return GraphWeight_; }

	void SetGraphHeight(int Height) { GraphHeight_ = Height; }
	void SetGraphWeight(int Weight) { GraphWeight_ = Weight; }
	std::string* GetGraphPtr() { return Graph_; }

	Object& SetX(int X) { X_ = X; return *this; }
	Object& SetY(int Y) { Y_ = Y; return *this; }

	void Transform()
	{
		std::string *NewPtr = new string[GetGraphWeight()];
		for (int i = 0; i < GetGraphWeight(); i++)
		{
			NewPtr[i] = string(GetGraphHeight(), '\0');
			for (int j = 0; j < GetGraphHeight(); j++)
				NewPtr[i][j] = GetGraphPtr()[GetGraphHeight() - 1 - j][i];
		}
		delete[] Graph_;
		Graph_ = NewPtr;
		int OldX = GetGraphWeight();
		SetGraphWeight(GetGraphHeight());
		SetGraphHeight(OldX);
		while (Y_ + GraphWeight_ > MAX_MAP_WEIGHT - 1)
			Y_--;
		while (Y_ + GraphWeight_ < 0)
			Y_++;
	}
};

class LObject1 : public Object
{
public:
	LObject1() : Object(-1, 5, 2, 3)
	{
		std::string *GraphPtr = GetGraphPtr();
		GraphPtr[0] = "  *";
		GraphPtr[1] = "***";
	}
};

class LObject2 : public Object
{
public:
	LObject2() : Object(-1, 5, 2, 3)
	{
		std::string *GraphPtr = GetGraphPtr();
		GraphPtr[0] = "*  ";
		GraphPtr[1] = "***";
	}
};

class TObject : public Object
{
public:
	TObject() : Object(-1, 5, 2, 3)
	{
		std::string *GraphPtr = GetGraphPtr();
		GraphPtr[0] = " * ";
		GraphPtr[1] = "***";
	}
};

class LineObject : public Object
{
public:
	LineObject() : Object(-1, 5, 1, 4)
	{
		std::string *GraphPtr = GetGraphPtr();
		GraphPtr[0] = "****";
	}
};

class ZObject1 : public Object
{
public:
	ZObject1() : Object(-1, 5, 2, 3)
	{
		std::string *GraphPtr = GetGraphPtr();
		GraphPtr[0] = "** ";
		GraphPtr[1] = " **";
	}
};

class ZObject2 : public Object
{
public:
	ZObject2() : Object(-1, 5, 2, 3)
	{
		std::string *GraphPtr = GetGraphPtr();
		GraphPtr[0] = " **";
		GraphPtr[1] = "** ";
	}
};

class SQObject : public Object
{
public:
	SQObject() : Object(-1, 5, 2, 2)
	{
		std::string *GraphPtr = GetGraphPtr();
		GraphPtr[0] = "**";
		GraphPtr[1] = "**";
	}
};

Object CreateNewObject()
{
	srand(time(NULL));
	int rnd = rand() % 7;
	switch (rnd)
	{
	case 0:
		return LObject1();
	case 1:
		return LObject2();
	case 2:
		return ZObject1();
	case 3:
		return ZObject2();
	case 4:
		return SQObject();
	case 5:
		return LineObject();
	case 6:
		return TObject();
	}
}

class TerisGameInfo
{
private:
	Object *CurrentControlledObj_;
	HANDLE Output_;
	ostream &out;

	class MovePoint {
	public:
		int X, Y;
	};

	void SetPosition(MovePoint& Point)
	{
		if (Point.X < 0 || Point.Y < 0) return;
		COORD Pos = { Point.Y, Point.X };
		SetConsoleCursorPosition(Output_, Pos);
	}

	static bool CheckXCollide(int pX, int pY, Object* Obj)
	{
		bool *check = new bool[Obj->GetGraphWeight()];
		for (int i = 0; i < Obj->GetGraphWeight(); i++)
		{
			check[i] = true;
		}
		for (int j = 0; j < Obj->GetGraphWeight(); j++)
		{
			for (int i = Obj->GetGraphHeight() - 1; i >= 0; i--)
			{
				if (Obj->GetGraphPtr()[i][j] == '*')
				{
					if (pX + i < MAX_MAP_HEIGHT - 1 && ShowMap[pX + i][pY + j] == '*')return true;
					break;
				}
			}
		}
		return false;
	}

	static bool CheckYCollide(int pX, int pY, Object* Obj)
	{
		for (int i = 0; i < Obj->GetGraphHeight(); i++)
		{
			for (int j = 0; j < Obj->GetGraphWeight(); j++)
			{
				if ((pY - Obj->GetY()) < 0) // LEFT
				{
					if (Obj->GetGraphPtr()[i][j] == '*')
					{
						if (Obj->GetY() - 1 >= 0 && ShowMap[pX + i][Obj->GetY() - 1] == '*')
							return true;
						break;
					}
				}
				else // RIGHT
				{
					if (Obj->GetGraphPtr()[i][Obj->GetGraphWeight() - 1 - j] == '*')
					{
						if (ShowMap[pX + i][Obj->GetY() + Obj->GetGraphWeight()] == '*')
							return true;
						break;
					}
				}
			}
		}
		return false;
	}

	void ClearOriginalPosition()
	{
		std::mutex IOLock;
		IOLock.lock();
		for (int i = 0; i < CurrentControlledObj_->GetGraphHeight(); i++)
		{
			for (int j = 0; j < CurrentControlledObj_->GetGraphWeight(); j++)
			{
				MovePoint Pt{ i + CurrentControlledObj_->GetX(), j + CurrentControlledObj_->GetY() };
				if (Pt.X < 0 || CurrentControlledObj_->GetGraphPtr()[i][j] != '*')
					continue;
				SetPosition(Pt);
				out << ' ';
				ShowMap[Pt.X][Pt.Y] = ' ';
			}
		}
		IOLock.unlock();
	}

	bool IsValidValue(int Value, char Chr)
	{
		switch (Chr)
		{
		case 'X':
			return Value >= 0 && Value < CurrentControlledObj_->GetGraphHeight();
			break;
		case 'Y':
			return Value >= 0 && Value < CurrentControlledObj_->GetGraphWeight();
			break;
		}
		return true;
	}

public:
	TerisGameInfo(ostream &set) : CurrentControlledObj_(new LObject1()), out(set)
	{
		Output_ = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	Object *GetCurrentObject()
	{
		return CurrentControlledObj_;
	}

	void DisplayObject(KeyboardInput Input, int OldX, int OldY)
	{
		std::mutex IOLock;
		IOLock.lock();
		int OffsetX = CurrentControlledObj_->GetX() - OldX, OffsetY = CurrentControlledObj_->GetY() - OldY;
		for (int i = 0; i < CurrentControlledObj_->GetGraphHeight(); i++)
		{
			if (i + CurrentControlledObj_->GetX() < 0)continue;
			MovePoint Pt;
			for (int j = 0; j < CurrentControlledObj_->GetGraphWeight(); j++)
			{
				if (CurrentControlledObj_->GetGraphPtr()[i][j] == ' ')continue;
				Pt = { CurrentControlledObj_->GetX() + i, CurrentControlledObj_->GetY() + j };
				SetPosition(Pt);
				if (Pt.X >= 0)
					ShowMap[Pt.X][Pt.Y] = CurrentControlledObj_->GetGraphPtr()[i][j];
				out << CurrentControlledObj_->GetGraphPtr()[i][j];
			}
		}
		IOLock.unlock();
	}

	void StopContol(std::mutex &ObjectLock, bool SetNewObject = true)
	{
		ObjectLock.unlock();
		std::mutex Lock;
		Lock.lock();
		//delete CurrentControlledObj_;
		if (SetNewObject)
		{
			*CurrentControlledObj_ = CreateNewObject();
		}
		Lock.unlock();
	}

	bool CheckCollide(bool(*CheckMethod)(int, int, Object *), int NewX, int NewY, Object *Obj)
	{
		if (NewX < 0 || NewY < 0) return false;
		return CheckMethod(NewX, NewY, Obj);
	}

	void CheckFullAndEliminate()
	{
		bool Refresh = false;
	LABEL_START_CHECK_UP:
		int Eliminate = 0, Line = -1;
		for (int i = MAX_MAP_HEIGHT - 1; i >= 0; --i)
		{
			for (int j = 1; j < ShowMap[i].size() - 1; ++j)
			{
				if (ShowMap[i][j] != '*')
				{
					break;
				}
				if (j == ShowMap[i].size() - 2)
				{
					Refresh = true;
					Eliminate++;
					if (Line == -1)
						Line = i;
				}
			}
		}
		if (Eliminate)
		{
			for (int i = Line; i >= 0; --i)
			{
				if (i - Eliminate >= 0)
					ShowMap[i] = std::move(ShowMap[i - Eliminate]);
			}
			for (int i = Eliminate; i >= 0; --i)
				ShowMap[i] = "|            |";
			goto LABEL_START_CHECK_UP;
		}
		if (Refresh)
		{
			system("cls");
			for (int i = 0; i < MAX_MAP_HEIGHT; i++)
				out << ShowMap[i] << endl;
		}
	}

	void MoveCurrentObject(KeyboardInput Input)
	{
		ClearOriginalPosition();
		std::mutex ObjectLock;
		ObjectLock.lock();
		int OldPosX = CurrentControlledObj_->GetX(), OldPosY = CurrentControlledObj_->GetY();
		switch (Input)
		{
		case KeyboardInput::LEFT:
		{
			int NewY = CurrentControlledObj_->GetY() - 1;
			if (CurrentControlledObj_->GetY() <= 1 || CheckCollide(CheckYCollide, CurrentControlledObj_->GetX(), NewY, CurrentControlledObj_))
				break;
			CurrentControlledObj_->SetY(CurrentControlledObj_->GetY() - 1);
			break;
		}
		case KeyboardInput::RIGHT:
		{
			int NewY = CurrentControlledObj_->GetY() + CurrentControlledObj_->GetGraphWeight();
			if (CurrentControlledObj_->GetY() + CurrentControlledObj_->GetGraphWeight() >= MAX_MAP_WEIGHT - 1 || CheckCollide(CheckYCollide, CurrentControlledObj_->GetX(), NewY, CurrentControlledObj_))
				break;
			CurrentControlledObj_->SetY(CurrentControlledObj_->GetY() + 1);
			break;
		}
		case KeyboardInput::UP:
			//ClearOriginalPosition();
			CurrentControlledObj_->Transform();
			break;
		case KeyboardInput::DOWN:
		{
			int NewX = CurrentControlledObj_->GetX() + 1;
			if ((CurrentControlledObj_->GetX() + CurrentControlledObj_->GetGraphHeight() >= MAX_MAP_HEIGHT - 1) || CheckCollide(TerisGameInfo::CheckXCollide, NewX, CurrentControlledObj_->GetY(), CurrentControlledObj_))
			{
				DisplayObject(Input, OldPosX, OldPosY);
				CheckFullAndEliminate();
				StopContol(ObjectLock);
				return;
			}
			CurrentControlledObj_->SetX(NewX);
			break;
		}
		case KeyboardInput::SPACE:
		{
			//ClearOriginalPosition();
			int NewX = CurrentControlledObj_->GetX();
			while (NewX + CurrentControlledObj_->GetGraphHeight() < MAX_MAP_HEIGHT - 1 && !CheckCollide(TerisGameInfo::CheckXCollide, NewX + 1, CurrentControlledObj_->GetY(), CurrentControlledObj_))
			{
				NewX++;
			}
			CurrentControlledObj_->SetX(NewX);
			DisplayObject(Input, OldPosX, OldPosY);
			CheckFullAndEliminate();
			StopContol(ObjectLock);
			return;
		}
		default:
			//std::cout << "Unknown Movement: " << static_cast<int>(Input) << std::endl;
			break;
		}
		DisplayObject(Input, OldPosX, OldPosY);
		ObjectLock.unlock();
	}

	void MoveCurrentObject()
	{
		if (input_queue.empty())
			return;
		ClearOriginalPosition();
		std::mutex ObjectLock;
		ObjectLock.lock();
		KeyboardInput Input = static_cast<KeyboardInput>(input_queue.front());
		input_queue.pop();
		int OldPosX = CurrentControlledObj_->GetX(), OldPosY = CurrentControlledObj_->GetY();
		switch (Input)
		{
		case KeyboardInput::LEFT:
		{
			int NewY = CurrentControlledObj_->GetY() - 1;
			if (CurrentControlledObj_->GetY() <= 1 || CheckCollide(CheckYCollide, CurrentControlledObj_->GetX(), NewY, CurrentControlledObj_))
				break;
			CurrentControlledObj_->SetY(CurrentControlledObj_->GetY() - 1);
			break;
		}
		case KeyboardInput::RIGHT:
		{
			int NewY = CurrentControlledObj_->GetY() + CurrentControlledObj_->GetGraphWeight();
			if (CurrentControlledObj_->GetY() + CurrentControlledObj_->GetGraphWeight() >= MAX_MAP_WEIGHT - 1 || CheckCollide(CheckYCollide, CurrentControlledObj_->GetX(), NewY, CurrentControlledObj_))
				break;
			CurrentControlledObj_->SetY(CurrentControlledObj_->GetY() + 1);
			break;
		}
		case KeyboardInput::UP:
			//ClearOriginalPosition();
			CurrentControlledObj_->Transform();
			break;
		case KeyboardInput::DOWN:
		{
			int NewX = CurrentControlledObj_->GetX() + 1;
			if ((CurrentControlledObj_->GetX() + CurrentControlledObj_->GetGraphHeight() >= MAX_MAP_HEIGHT - 1) || CheckCollide(TerisGameInfo::CheckXCollide, NewX, CurrentControlledObj_->GetY(), CurrentControlledObj_))
			{
				DisplayObject(Input, OldPosX, OldPosY);
				CheckFullAndEliminate();
				StopContol(ObjectLock);
				return;
			}
			CurrentControlledObj_->SetX(NewX);
			break;
		}
		case KeyboardInput::SPACE:
		{
			//ClearOriginalPosition();
			int NewX = CurrentControlledObj_->GetX();
			while (NewX + CurrentControlledObj_->GetGraphHeight() < MAX_MAP_HEIGHT - 1 && !CheckCollide(TerisGameInfo::CheckXCollide, NewX + 1, CurrentControlledObj_->GetY(), CurrentControlledObj_))
			{
				NewX++;
			}
			CurrentControlledObj_->SetX(NewX);
			DisplayObject(Input, OldPosX, OldPosY);
			CheckFullAndEliminate();
			StopContol(ObjectLock);
			return;
		}
		default:
			//std::cout << "Unknown Movement: " << static_cast<int>(Input) << std::endl;
			break;
		}
		DisplayObject(Input, OldPosX, OldPosY);
		ObjectLock.unlock();
	}
};

TerisGameInfo Info(cout);

void UserControl()
{
	std::mutex TerisLock;
	int Input = 0;
	//input_queue.push(getch());
	//cout << "TEST : " << input_queue.empty() << endl;
	while (Input = getch())
	{
		TerisLock.lock();
		input_queue.push(Input);
		if (!input_queue.empty())
		{
			Info.MoveCurrentObject();
			//Info.MoveCurrentObject(static_cast<KeyboardInput>(input_queue.front()));
			//input_queue.pop();
		}
		TerisLock.unlock();
	}
}

void AutoControl()
{
	std::mutex TerisLock;
	while (1)
	{
		TerisLock.lock();
		input_queue.push((int)(KeyboardInput::DOWN));
		if (!input_queue.empty())
		{
			Info.MoveCurrentObject();
			//Info.MoveCurrentObject(KeyboardInput::DOWN);
			//input_queue.pop();
		}
		TerisLock.unlock();
		_sleep(1000);
	}
	cout << "Error\n";
}

int main()
{
	for (int i = 0; i < MAX_MAP_HEIGHT; i++)
		cout << ShowMap[i] << endl;

	std::thread UserThread(UserControl)/*, AutoControlThread(AutoControl)*/;
	AutoControl();
	//AutoControlThread.join();
	UserThread.join();

	return 0;
}

