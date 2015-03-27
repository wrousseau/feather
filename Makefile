# -- Paths ---------------
FRONT_END_PATH = front-end

SRC_PATH = src
PARSER_PATH = parser
LEXER_PATH = lexer
INC_PATH = include
OBJ_PATH = obj
EXE_PATH = exe

FILE = SyntaxTree.cpp Algorithms.cpp SimpleC_gram.cpp SimpleC_lex.cpp

# -- Macros -------------
CXX = g++

# -- Flags --------------
CXX_INC_FLAGS = -I$(FRONT_END_PATH)/$(INC_PATH)
C_CXX_FLAGS = -Wall -Winline -fmessage-length=0 -ggdb -fno-inline
CXXFLAGS = $(C_CXX_FLAGS) $(CXX_INC_FLAGS) $(LIB_INC_PATH)

# -- Libraries ---------
LIBS = -ll

PRODUCT = feather

SRC = $(addprefix ${SRC_PATH}/, $(FILE))
OBJ = $(addprefix ${OBJ_PATH}/, $(addsuffix .o, $(basename $(FILE))))

# -- Rules --------------
$(EXE_PATH)/$(PRODUCT): $(OBJ)
	@mkdir -p $(EXE_PATH)
	$(CXX) -o $@ $^ $(INC) $(LIBS)
	@echo "Done..."

$(OBJ_PATH)/%.o: $(FRONT_END_PATH)/$(SRC_PATH)/%.cpp
	@mkdir -p $(OBJ_PATH)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_PATH)/%.o: $(FRONT_END_PATH)/$(SRC_PATH)/%.cpp
	@mkdir -p $(OBJ_PATH)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(FRONT_END_PATH)/$(SRC_PATH)/%_gram.cpp: $(FRONT_END_PATH)/$(PARSER_PATH)/%.yy
	bison --debug -o $@ $<

$(FRONT_END_PATH)/$(SRC_PATH)/%_lex.cpp: $(FRONT_END_PATH)/$(LEXER_PATH)/%.lex
	flex -o $@ $<

clean :
	rm -rf $(OBJ)
	rm -rf $(EXE_PATH)/$(PRODUCT)
	rm -f $(FRONT_END_PATH)/$(SRC_PATH)/*_gram.cpp $(FRONT_END_PATH)/$(SRC_PATH)/*_lex.cpp
	@echo "Clean done..."
