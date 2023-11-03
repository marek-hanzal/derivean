import {type IInventory} from "@derivean/inventory";
import {type IProducer}  from "./IProducer";

export interface IProducerProcess {
    /**
     * Current inventory
     */
    inventory: IInventory;
    /**
     * Producer definition a computation is based on
     */
    producer: IProducer;
    /**
     * Date where production computation should happen
     */
    date: {
        from: string;
        to: string;
    };
}
