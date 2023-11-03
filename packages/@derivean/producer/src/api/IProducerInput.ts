import {type IResource} from "@derivean/resource";

export interface IProducerInput {
    /**
     * Resource on input
     */
    resource: IResource;
    /**
     * Amount of processed resource (subtracted by time from an inventory)
     */
    amount: number;
}
