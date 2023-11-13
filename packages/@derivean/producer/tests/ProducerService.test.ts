import {
    withInventoryContainer,
    withInventoryService
}                   from "@derivean/inventory";
import {type IItem} from "@derivean/item";
import {Container}  from "@use-pico/container";
import {DateTime}   from "@use-pico/i18n";
import {
    describe,
    expect,
    test
}                   from "vitest";
import {
    type IProducer,
    type IProducerProcess,
    withProducerContainer,
    withProducerService
}                   from "../src";

const TreeItem: IItem = {
    name: "tree",
};
const LeafsItem: IItem = {
    name: "leafs",
};
const LogItem: IItem = {
    name: "log",
};
const SawdustItem: IItem = {
    name: "sawdust",
};

const producer: IProducer = {
    time:   10,
    input:  [
        {
            item: TreeItem,
            amount: 2,
        },
        {
            item: LeafsItem,
            amount:   2,
        },
    ],
    output: [
        {
            item: LogItem,
            amount:   6,
        },
        {
            item: SawdustItem,
            amount:   100,
        },
    ],
};
const date = {
    from: DateTime.now().minus({
        second: 35,
    }).toISO(),
    to:   DateTime.now().toISO(),
};

const goodProducerProcess: IProducerProcess = {
    date,
    producer,
    inventory: {
        items: [
            {
                item: TreeItem,
                amount:   10,
            },
            {
                item: LeafsItem,
                amount:   10,
            },
            {
                item: LogItem,
                amount:   2,
            },
        ],
    },
};
const notEnoughItemProducerProcess: IProducerProcess = {
    date,
    producer,
    inventory: {
        items: [
            {
                item: TreeItem,
                amount: 2,
            },
            {
                item: LeafsItem,
                amount:   1,
            },
            {
                item: LogItem,
                amount:   2,
            },
        ],
    },
};
const missingItemProducerProcess: IProducerProcess = {
    date,
    producer,
    inventory: {
        items: [
            {
                item: LogItem,
                amount:   2,
            },
        ],
    },
};

describe("ProducerService", () => {
    const container = new Container();
    withProducerContainer(container);
    withInventoryContainer(container);
    const producerService = withProducerService.use(container);
    const inventoryService = withInventoryService.use(container);

    test("Number of cycles", () => {
        expect(Math.floor(producerService.cycles(goodProducerProcess))).toBe(3);
    });

    test("Production works", () => {
        const snapshot = producerService.process(goodProducerProcess);
        expect(snapshot.isLimit).toBeFalsy();
        expect(snapshot.isReady).toBeTruthy();

        const treeItem = inventoryService.itemOf(snapshot.inventory, TreeItem.name);
        const logItem = inventoryService.itemOf(snapshot.inventory, LogItem.name);
        const sawdustItem = inventoryService.itemOf(snapshot.inventory, SawdustItem.name);

        expect(treeItem.length).toBe(1);
        expect(logItem.length).toBe(1);
        expect(sawdustItem.length).toBe(1);

        expect(treeItem[0].amount).toBe(-6);
        expect(logItem[0].amount).toBe(18);
        expect(sawdustItem[0].amount).toBe(300);
    });

    test("Production not ready", () => {
        const snapshot = producerService.process({
            ...goodProducerProcess,
            producer: {
                ...goodProducerProcess.producer,
                time: 6000,
            }
        });
        expect(snapshot.isReady).toBeFalsy();
    });

    test("Production missing items", () => {
        const snapshot = producerService.process(missingItemProducerProcess);

        const treeItem = inventoryService.itemOf(snapshot.inventory, TreeItem.name);
        const logItem = inventoryService.itemOf(snapshot.inventory, LogItem.name);
        const sawdustItem = inventoryService.itemOf(snapshot.inventory, SawdustItem.name);

        expect(treeItem.length).toBe(0);
        expect(logItem.length).toBe(1);
        expect(sawdustItem.length).toBe(1);

        expect(logItem[0].amount).toBe(0);
        expect(sawdustItem[0].amount).toBe(0);
    });

    test("Production missing items - is limit", () => {
        const snapshot = producerService.process(missingItemProducerProcess);
        expect(snapshot.isLimit).toBeTruthy();
        expect(snapshot.isReady).toBeFalsy();
    });

    test("Production not enough items", () => {
        const snapshot = producerService.process(notEnoughItemProducerProcess);
        expect(snapshot.isLimit).toBeTruthy();
        expect(snapshot.isReady).toBeFalsy();

        const treeItem = inventoryService.itemOf(snapshot.inventory, TreeItem.name);
        const logItem = inventoryService.itemOf(snapshot.inventory, LogItem.name);
        const sawdustItem = inventoryService.itemOf(snapshot.inventory, SawdustItem.name);

        expect(treeItem.length).toBe(1);
        expect(logItem.length).toBe(1);
        expect(sawdustItem.length).toBe(1);

        expect(logItem[0].amount).toBe(0);
        expect(sawdustItem[0].amount).toBe(0);
    });
});
