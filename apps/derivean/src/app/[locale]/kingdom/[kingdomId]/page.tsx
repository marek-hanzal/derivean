import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {KingdomIcon}           from "@derivean/ui";
import {Page}                  from "@use-pico/client";
import {tv}                    from "@use-pico/translator";

export namespace Index {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function Index({params: {kingdomId}}: Index.Props) {
    const kingdom = await withKingdomRepository.use(container).withQuery.fetchOrThrow({where: {id: kingdomId}});

    return <Page
        icon={<KingdomIcon/>}
        text={{
            header: tv(kingdom)`Kingdom overview`,
        }}
    >
        buildings, ...
    </Page>;
}
