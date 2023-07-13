NAME = shell
FLAGS = -Wall -pedantic-errors
TESTS =

cl: $(NAME).c
	gcc -o $(NAME) $(FLAGS) -lm $(NAME).c

run: cl
	./$(NAME) $(TESTS)

clean:
	rm -f $(NAME)
