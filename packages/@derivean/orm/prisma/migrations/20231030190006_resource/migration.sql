-- CreateTable
CREATE TABLE "Resource"
(
	"id"   TEXT NOT NULL,
	"name" TEXT NOT NULL,

	CONSTRAINT "Resource_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "Producer"
(
	"id"   TEXT    NOT NULL,
	"name" TEXT    NOT NULL,
	"time" INTEGER NOT NULL,

	CONSTRAINT "Producer_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "ProducerInput"
(
	"id"         TEXT             NOT NULL,
	"producerId" TEXT             NOT NULL,
	"resourceId" TEXT             NOT NULL,
	"amount"     DOUBLE PRECISION NOT NULL,

	CONSTRAINT "ProducerInput_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "ProducerOutput"
(
	"id"         TEXT             NOT NULL,
	"producerId" TEXT             NOT NULL,
	"resourceId" TEXT             NOT NULL,
	"amount"     DOUBLE PRECISION NOT NULL,

	CONSTRAINT "ProducerOutput_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE UNIQUE INDEX "Resource_name_key" ON "Resource" ("name");

-- CreateIndex
CREATE UNIQUE INDEX "Producer_name_key" ON "Producer" ("name");

-- AddForeignKey
ALTER TABLE "ProducerInput"
	ADD CONSTRAINT "ProducerInput_producerId_fkey" FOREIGN KEY ("producerId") REFERENCES "Producer" ("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ProducerInput"
	ADD CONSTRAINT "ProducerInput_resourceId_fkey" FOREIGN KEY ("resourceId") REFERENCES "Resource" ("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ProducerOutput"
	ADD CONSTRAINT "ProducerOutput_producerId_fkey" FOREIGN KEY ("producerId") REFERENCES "Producer" ("id") ON DELETE CASCADE ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "ProducerOutput"
	ADD CONSTRAINT "ProducerOutput_resourceId_fkey" FOREIGN KEY ("resourceId") REFERENCES "Resource" ("id") ON DELETE CASCADE ON UPDATE CASCADE;
