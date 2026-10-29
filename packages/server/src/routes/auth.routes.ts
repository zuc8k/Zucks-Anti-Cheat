import type { FastifyInstance } from "fastify";
import { z } from "zod";
import bcrypt from "bcryptjs";
import { PrismaClient } from "@prisma/client";

const prisma = new PrismaClient();

const LoginSchema = z.object({ username: z.string(), password: z.string() });

export async function authRoutes(app: FastifyInstance) {
  // Bootstrap: create default admin if none exists
  const count = await prisma.user.count();
  if (count === 0) {
    const hash = await bcrypt.hash("admin", 10);
    await prisma.user.create({ data: { username: "admin", password: hash } });
    app.log.warn("Created default admin / admin — CHANGE IT");
  }

  app.post("/login", async (req, reply) => {
    const parsed = LoginSchema.safeParse(req.body);
    if (!parsed.success) return reply.code(400).send({ error: "bad body" });
    const { username, password } = parsed.data;
    const user = await prisma.user.findUnique({ where: { username } });
    if (!user || !(await bcrypt.compare(password, user.password)))
      return reply.code(401).send({ error: "invalid credentials" });
    const token = app.jwt.sign({ sub: user.id, role: user.role }, { expiresIn: "12h" });
    return { token, user: { id: user.id, username: user.username, role: user.role } };
  });

  app.get("/me", { preHandler: [app.authenticate] }, async (req: any) => {
    const id = req.user.sub;
    const u = await prisma.user.findUnique({ where: { id } });
    return { id: u?.id, username: u?.username, role: u?.role };
  });
}

// Decorate app with auth guard
declare module "fastify" {
  interface FastifyInstance { authenticate: (req: any, reply: any) => Promise<void> }
}