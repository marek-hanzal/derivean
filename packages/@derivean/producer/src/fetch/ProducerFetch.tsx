import {withFetch}         from "@use-pico/source-ui";
import {withProducerQuery} from "../query/withProducerQuery";

export const ProducerFetch = withFetch({withQuery: withProducerQuery});
