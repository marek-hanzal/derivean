import {withBuildingRepository} from "@derivean/building";
import {container}              from "@derivean/server";
import {BuildingIcon}           from "@derivean/ui";
import {
    t,
    tx
}                               from "@use-pico/i18n";
import {
    Breadcrumbs,
    HomeIcon,
    ListIcon,
    Page
}                               from "@use-pico/ui";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Index({params: {id}}: Index.Props) {
    const building = await withBuildingRepository.use(container).withQuery.fetchOrThrow({
        where: {
            id,
        }
    });

    return <Page
        icon={<BuildingIcon/>}
        text={{
            title: tx({values: building})`Building detail (title)`,
            header: t({values: building})`Building detail`,
        }}
        postfix={<Breadcrumbs
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/building/list",
                    label: t()`Building list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
    >
        {building.name}
    </Page>;
}
