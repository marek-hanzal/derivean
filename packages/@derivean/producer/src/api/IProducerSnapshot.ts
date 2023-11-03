export interface IProducerSnapshot {
    inventory: any;
    /**
     * Tells if given a production result is on its limits
     */
    isLimit: boolean;
    /**
     * Is there anything produced? (ready to pick up)
     */
    isReady: boolean;
}
