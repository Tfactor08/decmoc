# Stack-based Parsing

- STATUS: OPEN
- PRIORITY: 150
- TAGS: refactor,parser

Maybe using trees for expressions is too inefficient, what about stack-based parsing (shunting-yard algorithm)? Actually, turns out that there's really no reason to use trees since we don't perform any processing of the trees except evaluating it. So building a couple of stacks could do the job of representing the precedence of operations.
